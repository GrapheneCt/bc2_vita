/* so_util.c -- utils to load .so file into RX memory, search and resolve symbols, hook
 *
 * Copyright (C) 2021 Andy Nguyen, GrapheneCt
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <kernel.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "symtable.h"
#include "so_util.h"
#include "al_error.h"

int sceKernelFreeMemBlockForVM(SceUID mbId);

int so_hook_thumb(uintptr_t addr, uintptr_t dst)
{
	if (addr == 0 || dst == 0)
		return AL_ERROR_INVALID_ARGUMENT;

	addr &= ~1;
	if (addr & 2) {
		uint16_t nop = 0xbf00;
		memcpy((void *)addr, &nop, sizeof(nop));
		addr += 2;
	}
	uint32_t hook[2];
	hook[0] = 0xf000f8df; // LDR PC, [PC]
	hook[1] = dst;
	memcpy((void *)addr, hook, sizeof(hook));

	return AL_OK;
}

int so_hook_arm(uintptr_t addr, uintptr_t dst)
{
	if (addr == 0 || dst == 0)
		return AL_ERROR_INVALID_ARGUMENT;

	uint32_t hook[2];
	hook[0] = 0xe51ff004; // LDR PC, [PC, #-0x4]
	hook[1] = dst;
	memcpy((void *)addr, hook, sizeof(hook));

	return AL_OK;
}

int so_hook_thumb_sym(so_module *mod, const char *symbol, uintptr_t dst)
{
	if (symbol == NULL || mod == NULL)
		return AL_ERROR_INVALID_POINTER;

	uintptr_t sym;
	so_symbol(mod, symbol, &sym);

	return so_hook_thumb(sym, dst);
}

int so_hook_arm_sym(so_module *mod, const char *symbol, uintptr_t dst)
{
	if (symbol == NULL || mod == NULL)
		return AL_ERROR_INVALID_POINTER;

	uintptr_t sym;
	so_symbol(mod, symbol, &sym);

	return so_hook_arm(sym, dst);
}

int so_flush_caches(so_module *mod)
{
	if (mod == NULL)
		return AL_ERROR_INVALID_POINTER;

	sceKernelSyncVMDomain(mod->text_blockid, (void *)mod->text_base, mod->text_size);

	return AL_OK;
}

int so_load(so_module *mod, const char *filename)
{
	int res = 0;
	uintptr_t data_addr = 0;
	SceUID so_blockid;
	void *so_data, *prog_data;
	size_t so_size, exec_size;

	if (mod == NULL || filename == NULL)
		return AL_ERROR_INVALID_POINTER;

	memset(mod, 0, sizeof(so_module));

	SceUID fd = sceIoOpen(filename, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	so_size = sceIoLseek(fd, 0, SCE_SEEK_END);
	sceIoLseek(fd, 0, SCE_SEEK_SET);

	so_blockid = sceKernelAllocMemBlock("AL::SoUtil::FileMem", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, (so_size + 0xfff) & ~0xfff, NULL);
	if (so_blockid < 0)
		return so_blockid;

	sceKernelGetMemBlockBase(so_blockid, &so_data);

	sceIoRead(fd, so_data, so_size);
	sceIoClose(fd);

	if (memcmp(so_data, ELFMAG, SELFMAG) != 0) {
		res = AL_ERROR_SO_UTIL_INVALID_ELFMAG;
		goto err_free_so;
	}

	exec_size = ALIGN_MEM(so_size, 1 * 1024 * 1024);
	res = mod->text_blockid = sceKernelAllocMemBlockForVM("AL::SoUtil::RwxBlock", exec_size);
	if (res < 0)
		goto err_free_so;

	sceKernelGetMemBlockBase(mod->text_blockid, &prog_data);

	sceKernelOpenVMDomain();

	mod->ehdr = (Elf32_Ehdr *)so_data;
	mod->phdr = (Elf32_Phdr *)((uintptr_t)so_data + mod->ehdr->e_phoff);
	mod->shdr = (Elf32_Shdr *)((uintptr_t)so_data + mod->ehdr->e_shoff);

	mod->shstr = (char *)((uintptr_t)so_data + mod->shdr[mod->ehdr->e_shstrndx].sh_offset);

	for (int i = 0; i < mod->ehdr->e_phnum; i++) {
		if (mod->phdr[i].p_type == PT_LOAD) {
			size_t prog_size;

			if ((mod->phdr[i].p_flags & PF_X) == PF_X) {
				prog_size = ALIGN_MEM(mod->phdr[i].p_memsz, mod->phdr[i].p_align);

				mod->phdr[i].p_vaddr += (Elf32_Addr)prog_data;

				mod->text_base = mod->phdr[i].p_vaddr;
				mod->text_size = mod->phdr[i].p_memsz;

				data_addr = (uintptr_t)prog_data + prog_size;
			} else {
				if (data_addr == 0) {
					res = AL_ERROR_SO_UTIL_EXEC_SEG_MISSING;
					goto err_free_text;
				}

				mod->phdr[i].p_vaddr += (Elf32_Addr)mod->text_base;

				mod->data_base = mod->phdr[i].p_vaddr;
				mod->data_size = mod->phdr[i].p_memsz;
			}

			memcpy((void *)mod->phdr[i].p_vaddr, (void *)((uintptr_t)so_data + mod->phdr[i].p_offset), mod->phdr[i].p_filesz);
		}
	}

	for (int i = 0; i < mod->ehdr->e_shnum; i++) {
		char *sh_name = mod->shstr + mod->shdr[i].sh_name;
		uintptr_t sh_addr = mod->text_base + mod->shdr[i].sh_addr;
		size_t sh_size = mod->shdr[i].sh_size;
		if (strcmp(sh_name, ".dynamic") == 0) {
			mod->dynamic = (Elf32_Dyn *)sh_addr;
			mod->num_dynamic = sh_size / sizeof(Elf32_Dyn);
		} else if (strcmp(sh_name, ".dynstr") == 0) {
			mod->dynstr = (char *)sh_addr;
		} else if (strcmp(sh_name, ".dynsym") == 0) {
			mod->dynsym = (Elf32_Sym *)sh_addr;
			mod->num_dynsym = sh_size / sizeof(Elf32_Sym);
		} else if (strcmp(sh_name, ".rel.dyn") == 0) {
			mod->reldyn = (Elf32_Rel *)sh_addr;
			mod->num_reldyn = sh_size / sizeof(Elf32_Rel);
		} else if (strcmp(sh_name, ".rel.plt") == 0) {
			mod->relplt = (Elf32_Rel *)sh_addr;
			mod->num_relplt = sh_size / sizeof(Elf32_Rel);
		} else if (strcmp(sh_name, ".init_array") == 0) {
			mod->init_array = (void *)sh_addr;
			mod->num_init_array = sh_size / sizeof(void *);
		} else if (strcmp(sh_name, ".hash") == 0) {
			mod->hash = (void *)sh_addr;
		}
	}

	for (int i = 0; i < mod->num_dynamic; i++) {
		switch (mod->dynamic[i].d_tag) {
		case DT_SONAME:
			mod->soname = mod->dynstr + mod->dynamic[i].d_un.d_ptr;
			break;
		default:
			break;
		}
	}

	if (mod->dynamic == NULL ||
		mod->dynstr == NULL ||
		mod->dynsym == NULL ||
		mod->reldyn == NULL ||
		mod->relplt == NULL) {
			res = AL_ERROR_SO_UTIL_INCOMPLETE;
			goto err_free_data;
	}

	sceKernelFreeMemBlock(so_blockid);

	return AL_OK;

err_free_data:
err_free_text:
	sceKernelFreeMemBlockForVM(mod->text_blockid);
err_free_so:
	sceKernelFreeMemBlock(so_blockid);

	return res;
}

int so_relocate(so_module *mod)
{
	if (mod == NULL)
		return AL_ERROR_INVALID_POINTER;

	for (int i = 0; i < mod->num_reldyn + mod->num_relplt; i++) {
		Elf32_Rel *rel = i < mod->num_reldyn ? &mod->reldyn[i] : &mod->relplt[i - mod->num_reldyn];
		Elf32_Sym *sym = &mod->dynsym[ELF32_R_SYM(rel->r_info)];
		uintptr_t *ptr = (uintptr_t *)(mod->text_base + rel->r_offset);

		int type = ELF32_R_TYPE(rel->r_info);
		switch (type) {
		case R_ARM_ABS32:
			*ptr += mod->text_base + sym->st_value;
			break;

		case R_ARM_RELATIVE:
			*ptr += mod->text_base;
			break;

		case R_ARM_GLOB_DAT:
		case R_ARM_JUMP_SLOT:
		{
			if (sym->st_shndx != SHN_UNDEF)
				*ptr = mod->text_base + sym->st_value;
			break;
		}

		default:
			return AL_ERROR_SO_UTIL_UNKNOWN_RELOC_TYPE;
			break;
		}
	}

	return AL_OK;
}

int so_resolve(so_module *mod, DynLibFunction *funcs, int num_funcs, int taint_missing_imports)
{
	int found = 0;

	if (mod == NULL || funcs == NULL)
		return AL_ERROR_INVALID_POINTER;

	for (int i = 0; i < mod->num_reldyn + mod->num_relplt; i++) {
		Elf32_Rel *rel = i < mod->num_reldyn ? &mod->reldyn[i] : &mod->relplt[i - mod->num_reldyn];
		Elf32_Sym *sym = &mod->dynsym[ELF32_R_SYM(rel->r_info)];
		uintptr_t *ptr = (uintptr_t *)(mod->text_base + rel->r_offset);

		int type = ELF32_R_TYPE(rel->r_info);
		switch (type) {
		case R_ARM_GLOB_DAT:
		case R_ARM_JUMP_SLOT:
		{
			if (sym->st_shndx == SHN_UNDEF) {
				// make it crash for debugging
				if (taint_missing_imports)
					*ptr = rel->r_offset;

				//printf("  { \"%s\", (uintptr_t)&%s },\n", mod->dynstr + sym->st_name, mod->dynstr + sym->st_name);

				found = 0;

				for (int j = 0; j < num_funcs; j++) {
					if (strcmp(mod->dynstr + sym->st_name, funcs[j].symbol) == 0) {
						*ptr = funcs[j].func;
						found = 1;
						break;
					}
				}

#ifdef _DEBUG
				if (!found)
					printf("NOT FOUND  { \"%s\", (uintptr_t)&%s },\n", mod->dynstr + sym->st_name, mod->dynstr + sym->st_name);
#endif
			}

			break;
		}

		default:
			break;
		}
	}

	return AL_OK;
}

int so_initialize(so_module *mod)
{
	if (mod == NULL)
		return AL_ERROR_INVALID_POINTER;

	for (int i = 0; i < mod->num_init_array; i++) {
		if (mod->init_array[i])
			mod->init_array[i]();
	}

	return AL_OK;
}

int so_hash(const uint8_t *name, uint32_t *hash)
{
	if (name == NULL || hash == NULL)
		return AL_ERROR_INVALID_POINTER;

	uint64_t h = 0, g;
	while (*name) {
		h = (h << 4) + *name++;
		if ((g = (h & 0xf0000000)) != 0)
		h ^= g >> 24;
		h &= 0x0fffffff;
	}

	*hash = h;

	return AL_OK;
}

int so_symbol(so_module *mod, const char *symbol, uintptr_t *res)
{
	uint32_t hash = 0;

	if (mod == NULL || symbol == NULL || res == NULL)
		return AL_ERROR_INVALID_POINTER;

	if (mod->hash) {
		so_hash((const uint8_t *)symbol, &hash);
		uint32_t nbucket = mod->hash[0];
		uint32_t *bucket = &mod->hash[2];
		uint32_t *chain = &bucket[nbucket];
		for (int i = bucket[hash % nbucket]; i; i = chain[i]) {
			if (strcmp(mod->dynstr + mod->dynsym[i].st_name, symbol) == 0) {
				*res = mod->text_base + mod->dynsym[i].st_value;
				return AL_OK;
			}
		}
	} else {
		for (int i = 0; i < mod->num_dynsym; i++) {
			if (strcmp(mod->dynstr + mod->dynsym[i].st_name, symbol) == 0) {
				*res = mod->text_base + mod->dynsym[i].st_value;
				return AL_OK;
			}
		}
	}

	return AL_ERROR_SO_UTIL_SYMBOL_NOT_FOUND;
}
