#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

#include <kernel.h>

typedef struct {
	char *symbol;
	uintptr_t func;
} DynLibFunction;

typedef struct {
	SceUID mbId;
	unsigned int size;
	unsigned int count;

	DynLibFunction *funTable;
} Symtable;

int symt_load_deps();
int symt_create(Symtable *table, unsigned int size, uintptr_t *newlibFunctable);
int symt_append(Symtable *table, const char *symbol, uintptr_t func);
int symt_override(Symtable *table, const char *symbol, uintptr_t func);

#endif
