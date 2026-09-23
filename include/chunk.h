#ifndef CHUNK_H
#define CHUNK_H

#include "value.h"
#include "instructions.h"

#include "../vendor/rdesc/include/rdesc.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

struct str_pool;  /* defined in string_pool.h */


struct chunk_line {
	int line;
	int count;
};

#define T struct chunk_line, chunk_line
#include "../vendor/libfun/include/stack.h"

#define T uint8_t, chunk
#include "../vendor/libfun/include/stack.h"


/* Compiled chunk. */
struct chunk {
	/* clox instructions. */
	struct fstack_chunk chunk;
	struct fstack_chunk_line chunk_line;

	struct fstack_vals constants;
};


/* Compile a parse tree. */
struct chunk chunk_xcompile(struct rdesc_node);

/* Free the resources owned by the chunk. */
void chunk_destroy(struct chunk *);

/* Print out disassembled instructions.
 * Use zero `len` for no limit. */
void chunk_disassemble(const struct chunk *,
		       const struct str_pool *idents,
		       FILE *out,
		       size_t offset,
		       size_t len);

/* Push a new instruction. */
void chunk_xwrite_inst(struct chunk *, int line, struct inst);

/* Change an instruction. */
void chunk_overwrite_inst(struct chunk *, size_t offset, struct inst);

/* Returns the length of the chunk. */
size_t chunk_len(const struct chunk *);

/* Reads an instruction. */
struct inst chunk_read_inst(const struct chunk *, size_t offset);

/* Push a new constant. */
uint32_t chunk_xpush_constant(struct chunk *, const struct val *);


#endif
