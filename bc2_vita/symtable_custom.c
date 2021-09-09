/* symtable_custom.c -- custom function implementations
 *
 * Copyright (C) 2021 Andy Nguyen, GrapheneCt
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <kernel.h>
#include <net.h>

/* CUSTOM FUNC IMPL */

int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
#ifdef _DEBUG
	va_list list;
	char string[512];

	va_start(list, fmt);
	vsprintf(string, fmt, list);
	va_end(list);

	printf("__android_log_print: %s", string);
#endif
	return 0;
}

long lrand48()
{
	unsigned int res;
	sceKernelGetRandomNumber(&res, 4);
	return res;
}

int *__errno()
{
	return _sceLibcErrnoLoc();
}

char *getcwd(char *buf, size_t size)
{
	if (buf) {
		buf[0] = '\0';
		return buf;
	}
	return NULL;
}

char *inet_ntoa(void* in)
{
	char res[256];
	sceClibMemset(res, 0, sizeof(res));

	sceNetInetNtop(SCE_NET_AF_INET, in, res, sizeof(res));

	return res;
}
