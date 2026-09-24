#include "../include/builtin_functions.h"
#include "../include/common.h"
#include "../include/gc.h"
#include "../include/value.h"
#include "../include/vm.h"

#include <inttypes.h>
#include <stdio.h>


static struct val hello_world(struct vm *vm _unused,
			      struct val argv[] _unused,
			      uint32_t argc _unused)
{
	printf("Hello, world! Got %"PRIu32" arguments.\n", argc);

	return NIL_VAL;
}

static struct val gc_run(struct vm *vm,
			  struct val argv[] _unused,
			  uint32_t argc _unused)
{
	struct gc_result res = vm_gc_mark(vm);

	printf("== GC run ==\n"
	       "%zu of %zu strings\n"
	       "%zu of %zu objects\n"
	       "%zu of %zu global ids\n"
	       "\t are reachable\n",
	       (size_t) 0, (size_t) 0,
	       res.objs, fstack_objects_len(&vm->objects),
	       (size_t) 0, (size_t) 0);

	vm_gc_sweep(vm);
	return NIL_VAL;
}

struct builtin_function builtin_functions[] = {
	{ "hello_world", hello_world },
	{ "gc_run", gc_run },
};

size_t builtin_functions_len = 2;
