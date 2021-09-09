/* dialog.c -- UI dialog/OSK
 *
 * Copyright (C) 2021 GrapheneCt
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <kernel.h>
#include <appmgr.h>
#include <libsysmodule.h>
#include <incoming_dialog.h>
#include <ces.h>

int dlg_show_idlg_error(const char *fmt, ...)
{
	uint32_t inSize, outSize;
	va_list list;
	char string[64];
	char tid[0x10];
	int state = SCE_INCOMING_DIALOG_STATUS_NONE;

	va_start(list, fmt);
	sceClibVsnprintf(string, 64, fmt, list);
	va_end(list);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_INCOMING_DIALOG))
		sceSysmoduleLoadModule(SCE_SYSMODULE_INCOMING_DIALOG);

	sceIncomingDialogInit(0);

	SceIncomingDialogParam params;
	sceIncomingDialogParamInit(&params);
	sceAppMgrAppParamGetString(SCE_KERNEL_PROCESS_ID_SELF, 12, tid, sizeof(tid));
	sceClibStrncpy((char *)params.titleId, tid, sizeof(params.titleId));
	params.timeout = 0x7FFFFFF0;

	SceCesUcsContext context;

	sceCesUcsContextInit(&context);
	sceCesUtf8StrToUtf16Str(
		&context,
		"OK",
		6,
		&inSize,
		(uint16_t *)params.acceptText,
		32,
		&outSize);

	sceCesUcsContextInit(&context);
	sceCesUtf8StrToUtf16Str(
		&context,
		string,
		64,
		&inSize,
		(uint16_t *)params.dialogText,
		64,
		&outSize);

	sceIncomingDialogOpen(&params);
	sceIncomingDialogSwitchToDialog();

	while (state != SCE_INCOMING_DIALOG_STATUS_ACCEPTED && state != SCE_INCOMING_DIALOG_STATUS_TIMEOUT) {
		state = sceIncomingDialogGetStatus();
		sceKernelDelayThread(10000);
	}

	sceAppMgrDestroyAppByAppId(-2);
}