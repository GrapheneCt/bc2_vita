/* main.c -- Battlefield: Bad Company 2 .so loader
 *
 * Copyright (C) 2021 Andy Nguyen, GrapheneCt
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <kernel.h>
#include <kernel/rng.h>
#include <audioout.h>
#include <ctrl.h>
#include <touch.h>
#include <libsysmodule.h>
#include <display.h>
#include <power.h>
#include <kubridge.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <math.h>

#include <errno.h>
#include <ctype.h>

#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <gpu_es4/psp2_pvr_hint.h>

#include "main.h"
#include "config.h"
#include "so_util.h"
#include "fs_overlay.h"

static uintptr_t *functable = NULL;

static so_module bc2_mod;

int ret0(void) {
	return 0;
}

int ret1(void) {
	return 1;
}

enum {
	ACTION_DOWN = 1,
	ACTION_UP   = 2,
	ACTION_MOVE = 3,
};

enum {
	AKEYCODE_DPAD_UP = 19,
	AKEYCODE_DPAD_DOWN = 20,
	AKEYCODE_DPAD_LEFT = 21,
	AKEYCODE_DPAD_RIGHT = 22,
	AKEYCODE_A = 29,
	AKEYCODE_B = 30,
	AKEYCODE_BUTTON_X = 99,
	AKEYCODE_BUTTON_Y = 100,
	AKEYCODE_BUTTON_L1 = 102,
	AKEYCODE_BUTTON_R1 = 103,
	AKEYCODE_BUTTON_START = 108,
	AKEYCODE_BUTTON_SELECT = 109,
};

typedef struct {
	uint32_t sce_button;
	uint32_t android_button;
} ButtonMapping;

static ButtonMapping mapping[] = {
	{ SCE_CTRL_UP,        AKEYCODE_DPAD_UP },
	{ SCE_CTRL_DOWN,      AKEYCODE_DPAD_DOWN },
	{ SCE_CTRL_LEFT,      AKEYCODE_DPAD_LEFT },
	{ SCE_CTRL_RIGHT,     AKEYCODE_DPAD_RIGHT },
	{ SCE_CTRL_CROSS,     AKEYCODE_A },
	{ SCE_CTRL_CIRCLE,    AKEYCODE_B },
	{ SCE_CTRL_SQUARE,    AKEYCODE_BUTTON_X },
	{ SCE_CTRL_TRIANGLE,  AKEYCODE_BUTTON_Y },
	{ SCE_CTRL_L,        AKEYCODE_BUTTON_L1 },
	{ SCE_CTRL_R,        AKEYCODE_BUTTON_R1 },
	{ SCE_CTRL_START,     AKEYCODE_BUTTON_START },
	{ SCE_CTRL_SELECT,    AKEYCODE_BUTTON_SELECT },
};

int ctrl_thread(SceSize args, void *argp)
{
	int (* Android_Karisma_AppOnTouchEvent)(int type, int x, int y, int id);
	int (* Android_Karisma_AppOnJoystickEvent)(int type, int x, int y, int id);
	int (* Android_Karisma_AppOnKeyEvent)(int type, int keycode);

	so_symbol(&bc2_mod, "Android_Karisma_AppOnTouchEvent", (uintptr_t *)&Android_Karisma_AppOnTouchEvent);
	so_symbol(&bc2_mod, "Android_Karisma_AppOnJoystickEvent", (uintptr_t *)&Android_Karisma_AppOnJoystickEvent);
	so_symbol(&bc2_mod, "Android_Karisma_AppOnKeyEvent", (uintptr_t *)&Android_Karisma_AppOnKeyEvent);

	int lastX[2] = { -1, -1 };
	int lastY[2] = { -1, -1 };

	float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f;
	uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0, released_buttons = 0;
	int ia1, ia2;

	while (1) {
		SceTouchData touch;
		sceTouchPeek(SCE_TOUCH_PORT_FRONT, &touch, 1);

		for (int i = 0; i < 2; i++) {
			if (i < touch.reportNum) {
				int x = (int)((float)touch.report[i].x * (float)SCREEN_W / 1920.0f);
				int y = (int)((float)touch.report[i].y * (float)SCREEN_H / 1088.0f);

				if (lastX[i] != -1 || lastY[i] != -1)
					Android_Karisma_AppOnTouchEvent(ACTION_DOWN, x, y, i);
				else
					Android_Karisma_AppOnTouchEvent(ACTION_MOVE, x, y, i);
				lastX[i] = x;
				lastY[i] = y;
			} else {
				if (lastX[i] != -1 || lastY[i] != -1)
					Android_Karisma_AppOnTouchEvent(ACTION_UP, lastX[i], lastY[i], i);
				lastX[i] = -1;
				lastY[i] = -1;
			}
		}

		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);

		old_buttons = current_buttons;
		current_buttons = pad.buttons;
		pressed_buttons = current_buttons & ~old_buttons;
		released_buttons = ~current_buttons & old_buttons;

		for (int i = 0; i < sizeof(mapping) / sizeof(ButtonMapping); i++) {
		if (pressed_buttons & mapping[i].sce_button)
			Android_Karisma_AppOnKeyEvent(0, mapping[i].android_button);
		if (released_buttons & mapping[i].sce_button)
			Android_Karisma_AppOnKeyEvent(1, mapping[i].android_button);
		}

		lx = ((float)pad.lx - 128.0f) / 128.0f;
		ly = ((float)pad.ly - 128.0f) / 128.0f;
		rx = ((float)pad.rx - 128.0f) / 128.0f;
		ry = ((float)pad.ry - 128.0f) / 128.0f;

		if (fabsf(lx) < 0.25f)
			lx = 0.0f;
		if (fabsf(ly) < 0.25f)
			ly = 0.0f;
		if (fabsf(rx) < 0.25f)
			rx = 0.0f;
		if (fabsf(ry) < 0.25f)
			ry = 0.0f;

		// TODO: send stop event only once
		ia1 = *(int *)&lx;
		ia2 = *(int *)&ly;
		Android_Karisma_AppOnJoystickEvent(3, ia1, ia2, 0);
		ia1 = *(int *)&rx;
		ia2 = *(int *)&ry;
		Android_Karisma_AppOnJoystickEvent(3, ia1, ia2, 1);

		sceDisplayWaitVblankStart();
	}

	return 0;
}

static int audio_port = 0;
static int disable_sound = 0;

void SetShortArrayRegion(void *env, int array, size_t start, size_t len, const uint8_t *buf)
{
	sceAudioOutOutput(audio_port, buf);
}

int sound_thread(SceSize args, void *argp)
{
	int(*Java_com_dle_bc2_KarismaBridge_nativeUpdateSound)(void *env, int unused, int type, size_t length);

	so_symbol(&bc2_mod, "Java_com_dle_bc2_KarismaBridge_nativeUpdateSound", (uintptr_t *)&Java_com_dle_bc2_KarismaBridge_nativeUpdateSound);

	audio_port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM, AUDIO_SAMPLES_PER_BUF / 2, AUDIO_SAMPLE_RATE, SCE_AUDIO_OUT_PARAM_FORMAT_S16_STEREO);

	static char fake_env[0x1000];
	memset(fake_env, 'A', sizeof(fake_env));
	*(uintptr_t *)(fake_env + 0x00) = (uintptr_t)fake_env; // just point to itself...
	*(uintptr_t *)(fake_env + 0x348) = (uintptr_t)SetShortArrayRegion;

	while (1) {
		if (disable_sound)
			sceKernelDelayThread(1000);
		else
			Java_com_dle_bc2_KarismaBridge_nativeUpdateSound(fake_env, 0, 0, AUDIO_SAMPLES_PER_BUF);
	}

	return 0;
}

static EGLDisplay dpy;
static EGLSurface surface;

int eglInit(EGLNativeDisplayType eglDisplay, EGLNativeWindowType eglWindow)
{
	EGLContext context;
	EGLConfig configs[2];
	EGLBoolean eRetStatus;
	EGLint major, minor;
	EGLint config_count;

	EGLint cfg_attribs[] = { EGL_BUFFER_SIZE,    EGL_DONT_CARE,
							EGL_RED_SIZE,       8,
							EGL_GREEN_SIZE,     8,
							EGL_BLUE_SIZE,      8,
							EGL_ALPHA_SIZE,		8,
							EGL_DEPTH_SIZE,		16,
							EGL_SAMPLES,		4,
							EGL_NONE };

	dpy = eglGetDisplay(eglDisplay);

	eRetStatus = eglInitialize(dpy, &major, &minor);

	if (eRetStatus != EGL_TRUE)
	{
		printf("Error: eglInitialize\n");
	}

	eRetStatus = eglGetConfigs(dpy, configs, 2, &config_count);

	if (eRetStatus != EGL_TRUE)
	{
		printf("Error: eglGetConfigs\n");
	}

	eRetStatus = eglChooseConfig(dpy, cfg_attribs, configs, 2, &config_count);

	if (eRetStatus != EGL_TRUE)
	{
		printf("Error: eglChooseConfig\n");
	}

	Psp2NativeWindow win;
	win.type = PSP2_DRAWABLE_TYPE_WINDOW;
	if (sceKernelGetModel() == SCE_KERNEL_MODEL_VITATV) {
		win.windowSize = PSP2_WINDOW_1920X1088;
		scePowerSetGpuClockFrequency(222);
	}
	else {
		win.windowSize = PSP2_WINDOW_960X544;
	}
	win.numFlipBuffers = 2;
	win.flipChainThrdAffinity = SCE_KERNEL_CPU_MASK_USER_1;

	surface = eglCreateWindowSurface(dpy, configs[0], &win, NULL);

	if (surface == EGL_NO_SURFACE)
	{
		printf("Error: eglCreateWindowSurface\n");
	}

	context = eglCreateContext(dpy, configs[0], EGL_NO_CONTEXT, NULL);

	if (context == EGL_NO_CONTEXT)
	{
		printf("Error: eglCreateContext\n");
	}

	eRetStatus = eglMakeCurrent(dpy, surface, surface, context);

	if (eRetStatus != EGL_TRUE)
	{
		printf("Error: eglMakeCurrent\n");
	}

	return 0;
}

int main_thread(SceSize args, void *argp)
{

	PVRSRV_PSP2_APPHINT hint;

	PVRSRVInitializeAppHint(&hint);

	hint.bDisableHWTextureUpload = 1;
	hint.bDisableHWTQBufferBlit = 1;
	hint.bDisableHWTQNormalBlit = 1;
	hint.bDisableHWTQTextureUpload = 1;
	hint.ui32UNCTexHeapSize = 60 * 1024 * 1024;

	PVRSRVCreateVirtualAppHint(&hint);

	eglInit(EGL_DEFAULT_DISPLAY, 0);

	int(*Android_Karisma_AppInit)(void);
	int(*Android_Karisma_InitGfxContext)(void);
	int(*Android_Karisma_AppUpdate)(void);

	so_symbol(&bc2_mod, "Android_Karisma_AppInit", (uintptr_t *)&Android_Karisma_AppInit);
	so_symbol(&bc2_mod, "Android_Karisma_InitGfxContext", (uintptr_t *)&Android_Karisma_InitGfxContext);
	so_symbol(&bc2_mod, "Android_Karisma_AppUpdate", (uintptr_t *)&Android_Karisma_AppUpdate);

	Android_Karisma_InitGfxContext();
	Android_Karisma_AppInit();

	SceUID ctrl_thid = sceKernelCreateThread("ctrl_thread", (SceKernelThreadEntry)ctrl_thread, 70, 128 * 1024, 0, SCE_KERNEL_CPU_MASK_USER_1, NULL);
	sceKernelStartThread(ctrl_thid, 0, NULL);

	SceUID sound_thid = sceKernelCreateThread("sound_thread", (SceKernelThreadEntry)sound_thread, 64, 128 * 1024, 0, SCE_KERNEL_CPU_MASK_USER_2, NULL);
	sceKernelStartThread(sound_thid, 0, NULL);

	while (1) {
		glEnable(GL_MULTISAMPLE);
		Android_Karisma_AppUpdate();
		eglSwapBuffers(dpy, surface);
	}

	return 0;
}

char *Android_KarismaBridge_GetAppReadPath(void)
{
	return DATA_PATH;
}

char *Android_KarismaBridge_GetAppWritePath(void)
{
	return DATA_PATH;
}

void Android_KarismaBridge_EnableSound(void)
{
	disable_sound = 0;
}

void Android_KarismaBridge_DisableSound(void)
{
	disable_sound = 1;
}

typedef struct {
	void *vtable;
	char *path;
	size_t pathLen;
} CPath;

int krm__krt__io__CPath__IsRoot(CPath **this)
{
	char *path = (*this)->path;
	if (strcmp(path, "ux0:") == 0)
		return 1;
	else
		return 0;
}

void patch_game(void)
{
	int *_ZN3krm3sal12SCREEN_WIDTHE;
	int *_ZN3krm3sal13SCREEN_HEIGHTE;

	so_symbol(&bc2_mod, "_ZN3krm3sal12SCREEN_WIDTHE", (uintptr_t *)&_ZN3krm3sal12SCREEN_WIDTHE);
	so_symbol(&bc2_mod, "_ZN3krm3sal13SCREEN_HEIGHTE", (uintptr_t *)&_ZN3krm3sal13SCREEN_HEIGHTE);

	*_ZN3krm3sal12SCREEN_WIDTHE = SCREEN_W;
	*_ZN3krm3sal13SCREEN_HEIGHTE = SCREEN_H;

	so_hook_arm_sym(&bc2_mod, "_ZN3krm10krtNetInitEv", (uintptr_t)&ret0);
	so_hook_arm_sym(&bc2_mod, "_ZN3krm3krt3dbg15krtDebugMgrInitEPNS0_16CApplicationBaseE", (uintptr_t)&ret0);

	so_hook_arm_sym(&bc2_mod, "_ZNK3krm3krt2io5CPath6IsRootEv", (uintptr_t)&krm__krt__io__CPath__IsRoot);

	so_hook_thumb_sym(&bc2_mod, "Android_KarismaBridge_GetAppReadPath", (uintptr_t)&Android_KarismaBridge_GetAppReadPath);
	so_hook_thumb_sym(&bc2_mod, "Android_KarismaBridge_GetAppWritePath", (uintptr_t)&Android_KarismaBridge_GetAppWritePath);

	so_hook_thumb_sym(&bc2_mod, "Android_KarismaBridge_GetKeyboardOpened", (uintptr_t)&ret0);

	so_hook_thumb_sym(&bc2_mod, "Android_KarismaBridge_EnableSound", (uintptr_t)&Android_KarismaBridge_EnableSound);
	so_hook_thumb_sym(&bc2_mod, "Android_KarismaBridge_DisableSound", (uintptr_t)&Android_KarismaBridge_DisableSound);
	so_hook_thumb_sym(&bc2_mod, "Android_KarismaBridge_LockSound", (uintptr_t)&ret0);
	so_hook_thumb_sym(&bc2_mod, "Android_KarismaBridge_UnlockSound", (uintptr_t)&ret0);
}

struct tm *localtime_hook(time_t *timer) {
	struct tm *res = localtime(timer);
	if (res)
		return res;
	// Fix an uninitialized variable bug.
	time(timer);
	return localtime(timer);
}

int check_kubridge(void) {
	int search_unk[2];
	//return _vshKernelSearchModuleByName("kubridge", search_unk);
	return 1;
}

int module_start(SceSize args, const void * argp)
{
	int ret = 0;

#ifdef LOAD_FROM_POSIX_BRIDGE
	if (argp == NULL) {
		sceClibPrintf("MAIN_MODULE not loaded from POSIX bridge!\n");
		abort();
	}

	functable = *(uintptr_t **)argp;

	if (functable == NULL) {
		sceClibPrintf("MAIN_MODULE not loaded from POSIX bridge!\n");
		abort();
	}
#endif

	if (check_kubridge() < 0)
		printf("Error kubridge.skprx is not installed.");

	sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITALANALOG_WIDE);
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, SCE_TOUCH_SAMPLING_STATE_START);

	Symtable table;

	ret = symt_load_deps();
	if (ret < 0)
		printf("symt_load_deps(): 0x%X\n", ret);

	ret = symt_create(&table, 4 * 1024, functable);
	if (ret < 0)
		printf("symt_create(): 0x%X\n", ret);

	symt_override(&table, "localtime", (uintptr_t)&localtime_hook);
	symt_override(&table, "printf", (uintptr_t)&ret0);
	symt_append(&table, "fcntl", (uintptr_t)&ret0);

	ret = fsov_create();
	if (ret < 0)
		printf("fsov_create(): 0x%X\n", ret);

	if (so_load(&bc2_mod, SO_PATH) < 0)
		printf("Error could not load %s.", SO_PATH);

	so_relocate(&bc2_mod);
	so_resolve(&bc2_mod, table.funTable, table.count, 1);

	patch_game();

	so_flush_caches(&bc2_mod);

	so_initialize(&bc2_mod);

	SceUID thid = sceKernelCreateThread("main_thread", (SceKernelThreadEntry)main_thread, 64, 128 * 1024, 0, SCE_KERNEL_CPU_MASK_USER_0, NULL);
	sceKernelStartThread(thid, 0, NULL);

	return SCE_KERNEL_START_SUCCESS;
}
