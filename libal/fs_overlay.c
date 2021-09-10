/* fs_overlay.c -- create filesystem overlay for safe operation
 *
 * Copyright (C) 2021 GrapheneCt
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <kernel.h>
#include <kernel/fios2.h>

#include "config.h"

int fsov_create()
{
	SceUID pid = sceKernelGetProcessId();
	SceFiosKernelOverlay ov;
	SceFiosKernelOverlayID ovId;
	sceClibMemset(&ov, 0, sizeof(SceFiosKernelOverlay));

	ov.type = SCE_FIOS_OVERLAY_TYPE_WRITABLE;
	ov.order = SCE_FIOS_OVERLAY_ORDER_USER_FIRST;
	ov.pid = pid;
	sceClibStrncpy(ov.src, SAVEDATA_PATH, sizeof(ov.src));
	sceClibStrncpy(ov.dst, DATA_PATH, sizeof(ov.dst));
	ov.src_len = sceClibStrnlen(ov.src, sizeof(ov.src));
	ov.dst_len = sceClibStrnlen(ov.dst, sizeof(ov.dst));

	return sceFiosKernelOverlayAddForProcess02(pid, &ov, &ovId);
}