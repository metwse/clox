#ifndef CHUNK_H
#define CHUNK_H

#include "value.h"
#include "instructions.h"

#include "../vendor/libfun/include/stack.h"
#include "../vendor/rdesc/include/rdesc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


/* Compiled chunk. */
struct chunk {
	/* clox instructions. */
	struct fstack chunk;
	struct fstack chunk_line_info;

	struct fstack constants;
};


/* Initialize a new chunk. */
void chunk_xinit(struct chunk *);

/* Free the resources owned by the chunk. */
void chunk_destroy(struct chunk *);

/* Print out disassembled instructions.
 * Use zero `len` for no limit. */
void chunk_disassemble(struct chunk *, FILE *out, size_t offset, size_t len);

/* Push a new instruction. */
void chunk_xwrite_inst(struct chunk *, int line, struct inst);

/* Reads an instruction. */
struct inst chunk_read_inst(const struct chunk *, size_t offset);

/* Compile a parse tree. */
void chunk_xcompile(struct chunk *, struct rdesc_node);

/* Push a new constant. */
uint32_t chunk_xpush_constant(struct chunk *, const struct val *);


#endif
