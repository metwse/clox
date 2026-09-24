#ifndef BUILTIN_FUNCTIONS
#define BUILTIN_FUNCTIONS

#include "object.h"

#include <stddef.h>


struct builtin_function {
	const char *const name;
	native_function_t *function;
};

extern struct builtin_function builtin_functions[];

extern size_t builtin_functions_len;


#endif
