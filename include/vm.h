#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "object.h"

#include "../vendor/libfun/include/hashmap.h"
#include "../vendor/libfun/include/stack.h"

#include <stddef.h>


struct call_frame {
	const struct chunk *c;  /* current chunk */
	size_t arity;  /* number of arguments */
	size_t pc;  /* program counter */
	size_t fp;  /* frame pointer */
};

/* The clox vm. */
struct vm {
	struct call_frame current;
	struct fstack frames;
	struct fstack stack;
	struct fstack objects;
	struct fhashmap globals;
};


/* Initialize a new VM. */
void vm_xinit(struct vm *);

/* Free the resources owned by the VM. */
void vm_destroy(struct vm *);

/* Execute the bytecode chunk. */
int vm_execute(struct vm *, const struct chunk *);

/* Track an object for garbage collection. */
void vm_obj_track(struct vm *, struct obj *);


#endif
