#ifndef __SO_UTIL_H__
#define __SO_UTIL_H__

#include "elf.h"
#include "symtable.h"

#define ALIGN_MEM(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#ifdef LOADER_USE_CDLG
#define RX_MEMBLOCK		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_RX
#define RW_MEMBLOCK		SCE_KERNEL_MEMBLOCK_TYPE_USER_CDIALOG_RW
#else
#define RX_MEMBLOCK		SCE_KERNEL_MEMBLOCK_TYPE_USER_RX
#define RW_MEMBLOCK		SCE_KERNEL_MEMBLOCK_TYPE_USER_RW
#endif

typedef struct {
	SceUID text_blockid, data_blockid;
	uintptr_t text_base, data_base;
	size_t text_size, data_size;

	Elf32_Ehdr *ehdr;
	Elf32_Phdr *phdr;
	Elf32_Shdr *shdr;

	Elf32_Dyn *dynamic;
	Elf32_Sym *dynsym;
	Elf32_Rel *reldyn;
	Elf32_Rel *relplt;

	int (** init_array)(void);
	uint32_t *hash;

	int num_dynamic;
	int num_dynsym;
	int num_reldyn;
	int num_relplt;
	int num_init_array;

	char *soname;
	char *shstr;
	char *dynstr;
} so_module;

int so_hook_thumb(uintptr_t addr, uintptr_t dst);
int so_hook_arm(uintptr_t addr, uintptr_t dst);
int so_hook_thumb_sym(so_module *mod, const char *symbol, uintptr_t dst);
int so_hook_arm_sym(so_module *mod, const char *symbol, uintptr_t dst);

int so_flush_caches(so_module *mod);
int so_load(so_module *mod, const char *filename);
int so_relocate(so_module *mod);
int so_resolve(so_module *mod, DynLibFunction *funcs, int num_funcs, int taint_missing_imports);
int so_initialize(so_module *mod);
int so_hash(const uint8_t *name, uint32_t *hash);
int so_symbol(so_module *mod, const char *symbol, uintptr_t *res);

#endif
