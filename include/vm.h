#ifndef VM_H
#define VM_H

#include "chunk.h"
#include "common.h"

#include "../vendor/libfun/include/stack.h"

#include <stddef.h>


/* The clox vm. */
struct vm {
	const struct chunk *current_chunk;
	size_t pc;

	struct fstack stack;
};


/* Initialize a new VM. */
void vm_xinit(struct vm *);

/* Free the resources owned by the VM. */
void vm_destroy(struct vm *);

/* Set the chunk to be interpreted. */
void vm_set_chunk(struct vm *, const struct chunk *);

/* Set the chunk to be interpreted. Returns non-zero if a runtime error
 * occured. */
int vm_run(struct vm *) _wur;


#endif
