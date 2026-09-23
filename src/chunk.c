#include "compiler_internal.h"

#include "../include/chunk.h"
#include "../include/instructions.h"
#include "../include/value.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


void chunk_xinit(struct chunk *c)
{
	fstack_chunk_xinit(&c->chunk);
	fstack_chunk_line_xinit(&c->chunk_line);
	fstack_vals_xinit(&c->constants);
}

void chunk_compact(struct chunk *c)
{
	fstack_chunk_shrink_to_fit(&c->chunk);
	fstack_chunk_line_shrink_to_fit(&c->chunk_line);
	fstack_vals_shrink_to_fit(&c->constants);
}

void chunk_destroy(struct chunk *c)
{
	fstack_chunk_destroy(&c->chunk);
	fstack_chunk_line_destroy(&c->chunk_line);

	/* TODO: GC will replace this.
	for (size_t i = 0; i < fstack_len(&c->constants); i++) {
		struct val v;
		v = *(struct val *) fstack_at(&c->constants, i);

		if (IS_OBJ(v) && AS_OBJ(v) != NULL)
			obj_free(AS_OBJ(v));
	}
	*/

	fstack_vals_destroy(&c->constants);
}

static void chunk_xwrite(struct chunk *c, int line, const uint8_t *chunk, size_t len)
{
	if (!len)
		return;

	/* TODO: multipush */
	for (size_t i = 0; i < len; i++) {
		if (chunk)
			fstack_chunk_xpush(&c->chunk, &chunk[i]);
		else
			fstack_chunk_xpush(&c->chunk, &(uint8_t) { 0 });
	}

	bool count_incremented = false;

	if (fstack_chunk_line_len(&c->chunk_line) > 0) {
		struct chunk_line *cli = fstack_chunk_line_top_mut(&c->chunk_line);

		if (cli->line == line) {
			cli->count += len;
			count_incremented = true;
		}
	}

	if (!count_incremented)
		fstack_chunk_line_xpush(&c->chunk_line,
					&(struct chunk_line) {
						.line = line,
						.count = len
					});
}

void chunk_xwrite_inst(struct chunk *c, int line, struct inst inst)
{
	chunk_xwrite(c, line, &(uint8_t) { inst.op }, 1);
	size_t arg_len = inst_len(inst) - 1;

	if (inst.args != NULL) {
		chunk_xwrite(c, line, inst.args, arg_len);
	} else {
		chunk_xwrite(c, line, NULL, arg_len);
	}
}

void chunk_overwrite_inst(struct chunk *c, size_t offset, struct inst inst)
{
	*fstack_chunk_at_mut(&c->chunk, offset) = inst.op;

	if (inst.args != NULL) {
		for (size_t i = 0; i < inst_len(inst) - 1; i++)
			*fstack_chunk_at_mut(&c->chunk, offset + i + 1) =
				((uint8_t *) inst.args)[i];
	}
}

size_t chunk_len(const struct chunk *c)
{
	return fstack_chunk_len(&c->chunk);
}

void chunk_disassemble(const struct chunk *c,
		       const struct str_pool *strings,
		       FILE *out,
		       size_t offset,
		       size_t len)
{
	if (fstack_chunk_len(&c->chunk) == 0)
		return;

	size_t cli_index = 0;
	size_t total_instructions = 0;
	int prev_line = -1;
	int line = -2;  /* no need to initialization, but due to false-positive
			 * uninitialized variable warning I set to -2 */

	for (size_t i = 0;
	     (len == 0 || i < len) && offset + i < fstack_chunk_len(&c->chunk);) {
		for (; total_instructions <= offset + i; cli_index++) {
			const struct chunk_line *cli =
				fstack_chunk_line_at(&c->chunk_line, cli_index);

			total_instructions += cli->count;
			line = cli->line;
		}

		struct inst inst = chunk_read_inst(c, offset + i);
		fprintf(out, "%04zu ", offset + i);
		i += inst_len(inst);

		inst_print(inst, out, prev_line == line ? -1 : line, strings);

		prev_line = line;
	}
}

struct inst chunk_read_inst(const struct chunk *c, size_t offset)
{
	const uint8_t *inst_bytes = fstack_chunk_at(&c->chunk, offset);

	return (struct inst) {
		.op = (enum opcode) inst_bytes[0],
		.args = (void *) (inst_bytes + 1)
	};
}

uint32_t chunk_xpush_constant(struct chunk *c, const struct val *v)
{
	uint32_t constant_id = fstack_vals_len(&c->constants);

	fstack_vals_xpush(&c->constants, v);

	return constant_id;
}
