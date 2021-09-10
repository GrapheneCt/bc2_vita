#ifndef __SYMTABLE_CUSTOM_H__
#define __SYMTABLE_CUSTOM_H__

static FILE __sF_fake[0x100][3];

int __android_log_print(int prio, const char *tag, const char *fmt, ...);
long lrand48();
int *__errno();
char *getcwd(char *buf, size_t size);
char *inet_ntoa(void* in);

#endif