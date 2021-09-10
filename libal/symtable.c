/* symtable.c -- symbol table for resolving
 *
 * Copyright (C) 2021 GrapheneCt
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

//
// STRUCTURE:
//
// ScePsp2Compat
//		|
//		|
// Trilithium POSIX
//		|
//		|
// newlib bridge OR custom impl
//

#include <kernel.h>
#include <kernel/rng.h>
#include <libsysmodule.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

#include "symtable.h"
#include "symtable_custom.h"
#include "so_util.h"
#include "al_error.h"

#if defined(SYMT_HAS_NEWLIB_)

#define SYMT_HAS_NEWLIB

#include "newlib_posix_bridge.h"
#endif

// base SCE
void __aeabi_atexit();
void __cxa_guard_acquire();
void __cxa_guard_release();
void __cxa_pure_virtual();
void __dso_handle();
void __stack_chk_fail();
void _ZdaPv();
void _ZdlPv();
void _Znaj();
void _Znwj();

void sceKernelLibcGettimeofday();

extern int __stack_chk_guard;

// libgcc
void __aeabi_d2f();
void __aeabi_d2ulz();
void __aeabi_dcmpgt();
void __aeabi_dmul();
void __aeabi_f2d();
void __aeabi_f2iz();
void __aeabi_f2ulz();
void __aeabi_fadd();
void __aeabi_fcmpge();
void __aeabi_fcmpgt();
void __aeabi_fcmple();
void __aeabi_fcmplt();
void __aeabi_fdiv();
void __aeabi_fsub();
void __aeabi_l2d();
void __aeabi_l2f();

void __aeabi_idiv();
void __aeabi_idivmod();
void __aeabi_uidiv();
void __aeabi_uidivmod();
void __aeabi_uldivmod();
void __aeabi_ldivmod();

#ifdef SYMT_HAS_PVR_PSP2_GLES1
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#include "sfp2hfp.h"

#ifdef SYMT_HAS_SCE_PSP2COMPAT
// ScePsp2Compat
void close();
void read();
void write();
void select();
void shutdown();
void getpid();
void recv();
void listen();
void closedir();
void sendto();
void bind();
void socket();
void opendir();
void recvfrom();
void accept();
void send();
void rmdir();
void mkdir();
void unlink();
void fstat();

int stat(const char *path, void *buf);

int lstat(const char *path, void *buf)
{
	return stat(path, buf);
}

int readdir_r(void* dirp, void* entry, void** result);

void *readdir(void *dirp)
{
	char res[0x100];
	void *result;

	readdir_r(dirp, res, &result);

	return result;
}

#endif

#ifdef SYMT_HAS_TRILITHIUM_POSIX
// Trilithium POSIX
void connect();
void gethostbyname();
void getsockname();
#endif

#ifdef SYMT_HAS_MONO_BRIDGE
#endif

#ifdef SYMT_HAS_PTHREAD
#endif

/* SYMT IMPL */

static char *newString(const char *s)
{
	unsigned int len = sceClibStrnlen(s, 256);
	char *ret = (char *)calloc(1, len + 1);
	strncpy(ret, s, len + 1);

	return ret;
}

int symt_append(Symtable *table, const char *symbol, uintptr_t func)
{
	if (table->count * sizeof(DynLibFunction) > table->size)
		return AL_ERROR_SYMT_TABLE_SIZE;

	table->funTable[table->count].symbol = newString(symbol);
	table->funTable[table->count].func = func;
	table->count++;

	return AL_OK;
}

int symt_create(Symtable *table, unsigned int size, uintptr_t *newlibFunctable)
{
	unsigned int alignedSize;

#ifdef SYMT_HAS_NEWLIB
	if (newlibFunctable == NULL)
		return AL_ERROR_INVALID_POINTER;
#endif

	if (table == NULL)
		return AL_ERROR_INVALID_POINTER;

	if (size == 0)
		size = SCE_KERNEL_4KiB;

	alignedSize = ALIGN_MEM(size, SCE_KERNEL_4KiB);

	table->mbId = sceKernelAllocMemBlock("AL::Symtable::Table", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, alignedSize, NULL);
	if (table->mbId <= 0)
		return table->mbId;

	table->size = alignedSize;
	table->count = 0;

	sceKernelGetMemBlockBase(table->mbId, &table->funTable);

	memset(table->funTable, 0, alignedSize);

	// SFP2HFP
	symt_append(table, "acos", (uintptr_t)&acos_sfp);
	symt_append(table, "asin", (uintptr_t)&asin_sfp);
	symt_append(table, "atan", (uintptr_t)&atan_sfp);
	symt_append(table, "atan2", (uintptr_t)&atan2_sfp);
	symt_append(table, "ceil", (uintptr_t)&ceil_sfp);
	symt_append(table, "cos", (uintptr_t)&cos_sfp);
	symt_append(table, "floor", (uintptr_t)&floor_sfp);
	symt_append(table, "fmod", (uintptr_t)&fmod_sfp);
	symt_append(table, "ldexp", (uintptr_t)&ldexp_sfp);
	symt_append(table, "log", (uintptr_t)&log_sfp);
	symt_append(table, "pow", (uintptr_t)&pow_sfp);
	symt_append(table, "sin", (uintptr_t)&sin_sfp);
	symt_append(table, "sqrt", (uintptr_t)&sqrt_sfp);
	symt_append(table, "tan", (uintptr_t)&tan_sfp);
	symt_append(table, "powf", (uintptr_t)&powf_sfp);

	// NORMAL
	symt_append(table, "__aeabi_atexit", (uintptr_t)&__aeabi_atexit);
	symt_append(table, "__cxa_guard_acquire", (uintptr_t)&__cxa_guard_acquire);
	symt_append(table, "__cxa_guard_release", (uintptr_t)&__cxa_guard_release);
	symt_append(table, "__cxa_pure_virtual", (uintptr_t)&__cxa_pure_virtual);
	symt_append(table, "__dso_handle", (uintptr_t)&__dso_handle);
	symt_append(table, "__stack_chk_fail", (uintptr_t)&__stack_chk_fail);
	symt_append(table, "_ZdaPv", (uintptr_t)&_ZdaPv);
	symt_append(table, "_ZdlPv", (uintptr_t)&_ZdlPv);
	symt_append(table, "_Znaj", (uintptr_t)&_Znaj);
	symt_append(table, "_Znwj", (uintptr_t)&_Znwj);
	symt_append(table, "difftime", (uintptr_t)&difftime);
	symt_append(table, "localtime", (uintptr_t)&localtime);
	symt_append(table, "fclose", (uintptr_t)&fclose);
	symt_append(table, "fflush", (uintptr_t)&fflush);
	symt_append(table, "fgets", (uintptr_t)&fgets);
	symt_append(table, "fileno", (uintptr_t)&fileno);
	symt_append(table, "fopen", (uintptr_t)&fopen);
	symt_append(table, "fprintf", (uintptr_t)&fprintf);
	symt_append(table, "fread", (uintptr_t)&fread);
	symt_append(table, "free", (uintptr_t)&free);
	symt_append(table, "fseek", (uintptr_t)&fseek);
	symt_append(table, "ftell", (uintptr_t)&ftell);
	symt_append(table, "fwrite", (uintptr_t)&fwrite);
	symt_append(table, "malloc", (uintptr_t)&malloc);
	symt_append(table, "memcmp", (uintptr_t)&memcmp);
	symt_append(table, "memcpy", (uintptr_t)&memcpy);
	symt_append(table, "memmove", (uintptr_t)&memmove);
	symt_append(table, "memset", (uintptr_t)&memset);
	symt_append(table, "printf", (uintptr_t)&printf);
	symt_append(table, "realloc", (uintptr_t)&realloc);
	symt_append(table, "snprintf", (uintptr_t)&snprintf);
	symt_append(table, "sscanf", (uintptr_t)&sscanf);
	symt_append(table, "strchr", (uintptr_t)&strchr);
	symt_append(table, "strcmp", (uintptr_t)&strcmp);
	symt_append(table, "strerror", (uintptr_t)&strerror);
	symt_append(table, "strlen", (uintptr_t)&strlen);
	symt_append(table, "strncat", (uintptr_t)&strncat);
	symt_append(table, "strncmp", (uintptr_t)&strncmp);
	symt_append(table, "strncpy", (uintptr_t)&strncpy);
	symt_append(table, "strrchr", (uintptr_t)&strrchr);
	symt_append(table, "strstr", (uintptr_t)&strstr);
	symt_append(table, "strtoll", (uintptr_t)&strtoll);
	symt_append(table, "time", (uintptr_t)&time);
	symt_append(table, "tolower", (uintptr_t)&tolower);
	symt_append(table, "toupper", (uintptr_t)&toupper);
	symt_append(table, "vsnprintf", (uintptr_t)&vsnprintf);
	symt_append(table, "abort", (uintptr_t)&abort);
	symt_append(table, "strcpy", (uintptr_t)&strcpy);
	symt_append(table, "atoi", (uintptr_t)&atoi);
	symt_append(table, "strcat", (uintptr_t)&strcat);

	symt_append(table, "gettimeofday", (uintptr_t)&sceKernelLibcGettimeofday);

	symt_append(table, "__aeabi_idiv", (uintptr_t)&__aeabi_idiv);
	symt_append(table, "__aeabi_idivmod", (uintptr_t)&__aeabi_idivmod);
	symt_append(table, "__aeabi_uidiv", (uintptr_t)&__aeabi_uidiv);
	symt_append(table, "__aeabi_uidivmod", (uintptr_t)&__aeabi_uidivmod);
	symt_append(table, "__aeabi_uldivmod", (uintptr_t)&__aeabi_uldivmod);
	symt_append(table, "__aeabi_ldivmod", (uintptr_t)&__aeabi_ldivmod);

	symt_append(table, "__aeabi_d2f", (uintptr_t)&__aeabi_d2f);
	symt_append(table, "__aeabi_d2ulz", (uintptr_t)&__aeabi_d2ulz);
	symt_append(table, "__aeabi_dcmpgt", (uintptr_t)&__aeabi_dcmpgt);
	symt_append(table, "__aeabi_dmul", (uintptr_t)&__aeabi_dmul);
	symt_append(table, "__aeabi_f2d", (uintptr_t)&__aeabi_f2d);
	symt_append(table, "__aeabi_f2iz", (uintptr_t)&__aeabi_f2iz);
	symt_append(table, "__aeabi_f2ulz", (uintptr_t)&__aeabi_f2ulz);
	symt_append(table, "__aeabi_fadd", (uintptr_t)&__aeabi_fadd);
	symt_append(table, "__aeabi_fcmpge", (uintptr_t)&__aeabi_fcmpge);
	symt_append(table, "__aeabi_fcmpgt", (uintptr_t)&__aeabi_fcmpgt);
	symt_append(table, "__aeabi_fcmple", (uintptr_t)&__aeabi_fcmple);
	symt_append(table, "__aeabi_fcmplt", (uintptr_t)&__aeabi_fcmplt);
	symt_append(table, "__aeabi_fdiv", (uintptr_t)&__aeabi_fdiv);
	symt_append(table, "__aeabi_fsub", (uintptr_t)&__aeabi_fsub);
	symt_append(table, "__aeabi_l2d", (uintptr_t)&__aeabi_l2d);
	symt_append(table, "__aeabi_l2f", (uintptr_t)&__aeabi_l2f);

#ifdef SYMT_HAS_SCE_PSP2COMPAT
	symt_append(table, "close", (uintptr_t)&close);
	symt_append(table, "read", (uintptr_t)&read);
	symt_append(table, "write", (uintptr_t)&write);
	symt_append(table, "select", (uintptr_t)&select);
	symt_append(table, "shutdown", (uintptr_t)&shutdown);
	symt_append(table, "getpid", (uintptr_t)&getpid);
	symt_append(table, "recv", (uintptr_t)&recv);
	symt_append(table, "listen", (uintptr_t)&listen);
	symt_append(table, "closedir", (uintptr_t)&closedir);
	symt_append(table, "sendto", (uintptr_t)&sendto);
	symt_append(table, "bind", (uintptr_t)&bind);
	symt_append(table, "socket", (uintptr_t)&socket);
	symt_append(table, "opendir", (uintptr_t)&opendir);
	symt_append(table, "recvfrom", (uintptr_t)&recvfrom);
	symt_append(table, "accept", (uintptr_t)&accept);
	symt_append(table, "send", (uintptr_t)&send);
	symt_append(table, "rmdir", (uintptr_t)&rmdir);
	symt_append(table, "mkdir", (uintptr_t)&mkdir);
	symt_append(table, "unlink", (uintptr_t)&unlink);
	symt_append(table, "readdir_r", (uintptr_t)&readdir_r);
	symt_append(table, "readdir", (uintptr_t)&readdir);
	symt_append(table, "stat", (uintptr_t)&stat);
	symt_append(table, "lstat", (uintptr_t)&lstat);
	symt_append(table, "fstat", (uintptr_t)&fstat);
#endif

#ifdef SYMT_HAS_TRILITHIUM_POSIX
	symt_append(table, "connect", (uintptr_t)&connect);
	symt_append(table, "gethostbyname", (uintptr_t)&gethostbyname);
	symt_append(table, "getsockname", (uintptr_t)&getsockname);
#endif

#ifdef SYMT_HAS_MONO_BRIDGE
#endif

#if defined(SYMT_HAS_PVR_PSP2_GLES1) || defined(SYMT_HAS_PVR_PSP2_GLES2)

	// SFP2HFP
	symt_append(table, "glAlphaFunc", (uintptr_t)&glAlphaFunc_sfp);
	symt_append(table, "glClearDepthf", (uintptr_t)&glClearDepthf_sfp);
	symt_append(table, "glDepthRangef", (uintptr_t)&glDepthRangef_sfp);
	symt_append(table, "glFogf", (uintptr_t)&glFogf_sfp);
	symt_append(table, "glTexEnvf", (uintptr_t)&glTexEnvf_sfp);

	// NORMAL
	symt_append(table, "glActiveTexture", (uintptr_t)&glActiveTexture);
	symt_append(table, "glBindBuffer", (uintptr_t)&glBindBuffer);
	symt_append(table, "glBindFramebufferOES", (uintptr_t)&glBindFramebufferOES);
	symt_append(table, "glBindTexture", (uintptr_t)&glBindTexture);
	symt_append(table, "glBlendFunc", (uintptr_t)&glBlendFunc);
	symt_append(table, "glClear", (uintptr_t)&glClear);
	symt_append(table, "glClearColor", (uintptr_t)&glClearColor);
	symt_append(table, "glClearStencil", (uintptr_t)&glClearStencil);
	symt_append(table, "glClientActiveTexture", (uintptr_t)&glClientActiveTexture);
	symt_append(table, "glColorMask", (uintptr_t)&glColorMask);
	symt_append(table, "glColorPointer", (uintptr_t)&glColorPointer);
	symt_append(table, "glCompressedTexImage2D", (uintptr_t)&glCompressedTexImage2D);
	symt_append(table, "glCullFace", (uintptr_t)&glCullFace);
	symt_append(table, "glDeleteBuffers", (uintptr_t)&glDeleteBuffers);
	symt_append(table, "glDeleteTextures", (uintptr_t)&glDeleteTextures);
	symt_append(table, "glDepthFunc", (uintptr_t)&glDepthFunc);
	symt_append(table, "glDepthMask", (uintptr_t)&glDepthMask);
	symt_append(table, "glDisable", (uintptr_t)&glDisable);
	symt_append(table, "glDisableClientState", (uintptr_t)&glDisableClientState);
	symt_append(table, "glDrawArrays", (uintptr_t)&glDrawArrays);
	symt_append(table, "glDrawElements", (uintptr_t)&glDrawElements);
	symt_append(table, "glEnable", (uintptr_t)&glEnable);
	symt_append(table, "glEnableClientState", (uintptr_t)&glEnableClientState);
	symt_append(table, "glFogfv", (uintptr_t)&glFogfv);
	symt_append(table, "glFrontFace", (uintptr_t)&glFrontFace);
	symt_append(table, "glGenTextures", (uintptr_t)&glGenTextures);
	symt_append(table, "glGetError", (uintptr_t)&glGetError);
	symt_append(table, "glGetIntegerv", (uintptr_t)&glGetIntegerv);
	symt_append(table, "glGetString", (uintptr_t)&glGetString);
	symt_append(table, "glLoadIdentity", (uintptr_t)&glLoadIdentity);
	symt_append(table, "glLoadMatrixf", (uintptr_t)&glLoadMatrixf);
	symt_append(table, "glMatrixMode", (uintptr_t)&glMatrixMode);
	symt_append(table, "glNormalPointer", (uintptr_t)&glNormalPointer);
	symt_append(table, "glReadPixels", (uintptr_t)&glReadPixels);
	symt_append(table, "glScissor", (uintptr_t)&glScissor);
	symt_append(table, "glStencilFunc", (uintptr_t)&glStencilFunc);
	symt_append(table, "glStencilOp", (uintptr_t)&glStencilOp);
	symt_append(table, "glTexCoordPointer", (uintptr_t)&glTexCoordPointer);
	symt_append(table, "glTexEnvfv", (uintptr_t)&glTexEnvfv);
	symt_append(table, "glTexImage2D", (uintptr_t)&glTexImage2D);
	symt_append(table, "glTexParameteri", (uintptr_t)&glTexParameteri);
	symt_append(table, "glVertexPointer", (uintptr_t)&glVertexPointer);
	symt_append(table, "glViewport", (uintptr_t)&glViewport);

	symt_append(table, "eglInitialize", (uintptr_t)&eglInitialize);
	symt_append(table, "eglSwapBuffers", (uintptr_t)&eglSwapBuffers);
	symt_append(table, "eglGetDisplay", (uintptr_t)&eglGetDisplay);
	symt_append(table, "eglChooseConfig", (uintptr_t)&eglChooseConfig);
	symt_append(table, "eglCreateWindowSurface", (uintptr_t)&eglCreateWindowSurface);
	symt_append(table, "eglCreateContext", (uintptr_t)&eglCreateContext);
	symt_append(table, "eglMakeCurrent", (uintptr_t)&eglMakeCurrent);
	symt_append(table, "eglQuerySurface", (uintptr_t)&eglQuerySurface);
	symt_append(table, "eglGetError", (uintptr_t)&eglGetError);
	symt_append(table, "eglDestroyContext", (uintptr_t)&eglDestroyContext);
	symt_append(table, "eglDestroySurface", (uintptr_t)&eglDestroySurface);
	symt_append(table, "eglTerminate", (uintptr_t)&eglTerminate);
	symt_append(table, "eglGetProcAddress", (uintptr_t)&eglGetProcAddress);
#endif

	// CUSTOM IMPL
	symt_append(table, "lrand48", (uintptr_t)&lrand48);
	symt_append(table, "__android_log_print", (uintptr_t)&__android_log_print);
	symt_append(table, "__errno", (uintptr_t)&__errno);
	symt_append(table, "getcwd", (uintptr_t)&getcwd);
	symt_append(table, "inet_ntoa", (uintptr_t)&inet_ntoa);

	// VARIABLES
	symt_append(table, "__stack_chk_guard", (uintptr_t)&__stack_chk_guard);
	symt_append(table, "__sF", (uintptr_t)&__sF_fake);
	
	return AL_OK;
}

int symt_override(Symtable *table, const char *symbol, uintptr_t func)
{
	if (table == NULL || symbol == NULL)
		return AL_ERROR_INVALID_POINTER;

	for (int i = 0; i < table->count; i++) {
		if (!strncmp(symbol, table->funTable[i].symbol, 256)) {
			table->funTable[i].func = func;
			return AL_OK;
		}
	}

	return AL_ERROR_SYMT_SYMBOL_NOT_FOUND;
}

int symt_load_deps()
{
	SceUID ret = SCE_UID_INVALID_UID;

#if defined(SYMT_HAS_PVR_PSP2_GLES1) || defined(SYMT_HAS_PVR_PSP2_GLES2)
	ret = sceKernelLoadStartModule("app0:module/libgpu_es4_ext.suprx", 0, NULL, 0, NULL, 0);
	if (ret <= 0)
		return ret;
	ret = sceKernelLoadStartModule("app0:module/libIMGEGL.suprx", 0, NULL, 0, NULL, 0);
	if (ret <= 0)
		return ret;
#endif

#ifdef SYMT_HAS_VITAGL
	ret = sceKernelLoadStartModule("app0:module/libvgl.suprx", 0, NULL, 0, NULL, 0);
	if (ret <= 0)
		return ret;
#endif

#ifdef SYMT_HAS_SCE_PSP2COMPAT
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
	ret = sceKernelLoadStartModule("vs0:data/external/webcore/ScePsp2Compat.suprx", 0, NULL, 0, NULL, 0);
	if (ret <= 0)
		return ret;
#endif

#ifdef SYMT_HAS_TRILITHIUM_POSIX
	ret = sceKernelLoadStartModule("app0:module/posix.suprx", 0, NULL, 0, NULL, 0);
	if (ret <= 0)
		return ret;
#endif

#ifdef SYMT_HAS_MONO_BRIDGE
	sceSysmoduleLoadModule(SCE_SYSMODULE_SSL);
	ret = sceKernelLoadStartModule("app0:module/libmono_bridge.suprx", 0, NULL, 0, NULL, 0);
	if (ret <= 0)
		return ret;
#endif

	return AL_OK;
}