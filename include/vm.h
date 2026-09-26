#ifndef VM_H
#define VM_H

#include "value.h"

#include <stddef.h>


struct call_frame {
	struct obj_closure *closure;
	const struct chunk *c;  /* current chunk */
	size_t arity;  /* number of arguments */
	size_t pc;  /* program counter */
	size_t fp;  /* frame pointer */
};

#define T struct call_frame, call_frames
#include "../vendor/libfun/include/stack.h"

#define T struct obj *, objects
#include "../vendor/libfun/include/stack.h"


/* The Lw vm. */
struct vm {
	struct str_pool *strings;
	struct globals *globals;

	struct call_frame current;
	struct fstack_call_frames frames;

	struct fstack_vals stack;
	struct fstack_objects objects;
	struct obj_upvalue *open_upvalues;
};


/* Initialize a new VM. */
void vm_xinit(struct vm *,
	      struct str_pool *strings,
	      struct globals *globals);

/* Free the resources owned by the VM. */
void vm_destroy(struct vm *);

/* Execute the bytecode chunk. */
int vm_execute(struct vm *, const struct chunk *);

/* Allocate a new object. */
struct obj *vm_obj_alloc(struct vm *, size_t size);


#endif
