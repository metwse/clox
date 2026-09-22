#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "object.h"
#include "string_pool.h"

#include <stddef.h>


struct call_frame {
	struct obj_closure *closure;
	const struct chunk *c;  /* current chunk */
	size_t arity;  /* number of arguments */
	size_t pc;  /* program counter */
	size_t fp;  /* frame pointer */
};

#define T uint32_t, struct val, globals
#include "../vendor/libfun/include/hmap.h"

#define T struct call_frame, call_frames
#include "../vendor/libfun/include/stack.h"


/* The clox vm. */
struct vm {
	struct str_pool *idents;
	struct str_pool *str_literals;

	struct call_frame current;
	struct fstack_call_frames frames;

	struct fstack_vals stack;
	/* struct fstack objects; */
	struct fhmap_globals globals;
	struct obj_upvalue *open_upvalues;
};


/* Initialize a new VM. */
void vm_xinit(struct vm *,
	      struct str_pool *idents,
	      struct str_pool *str_literals);

/* Free the resources owned by the VM. */
void vm_destroy(struct vm *);

/* Execute the bytecode chunk. */
int vm_execute(struct vm *, const struct chunk *);

/* Track an object for garbage collection. */
void vm_obj_track(struct vm *, struct obj *);


#endif
