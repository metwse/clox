#ifndef VM_H
#define VM_H

#include "instructions.h"

#include "../vendor/libfun/include/stack.h"

#include <stddef.h>
#include <stdio.h>


/* The clox vm. */
struct vm {
	/* clox instructions. */
	struct fstack chunk;
	struct fstack chunk_line_info;

	struct fstack constants;
	struct fstack stack;

	size_t pc  /* program counter */;
};


/* Initialize a new VM. */
void vm_xinit(struct vm *);

/* Free the resources owned by the VM. */
void vm_destroy(struct vm *);

/* Push new instructions. */
void vm_chunk_xwrite(struct vm *, int line, char *chunk, size_t len);

/* Push a new instruction. */
void vm_chunk_xwrite_inst(struct vm *, int line, struct inst);

/* Print out disassembled instructions.
 * Use 0 len for no limit. */
void vm_disassemble(struct vm *, FILE *out, size_t offset, size_t len);

/* Reads an instruction. */
struct inst vm_chunk_read_inst(struct vm *, size_t offset);


#endif
