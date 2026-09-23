#include "../include/builtin_functions.h"
#include "../include/common.h"
#include "../include/value.h"

#include <inttypes.h>
#include <stdio.h>


static struct val hello_world(struct vm *vm _unused,
			      struct val argv[] _unused,
			      uint32_t argc _unused)
{
	printf("Hello, world! Got %"PRIu32" arguments.\n", argc);

	return NIL_VAL;
}

struct builtin_function builtin_functions[] = {
	{ "hello_world", hello_world }
};

size_t builtin_functions_len = 1;
