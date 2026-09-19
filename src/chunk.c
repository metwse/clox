#include "../include/chunk.h"
#include "../include/instructions.h"
#include "../include/value.h"
#include "../include/object.h"

#include "../vendor/libfun/include/stack.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


struct chunk_line_info {
	int line;
	int count;
};


void chunk_xinit(struct chunk *c)
{
	fstack_xinit(&c->chunk, 1);
	fstack_xinit(&c->chunk_line_info, sizeof(struct chunk_line_info));

	fstack_xinit(&c->constants, sizeof(struct val));
}

void chunk_destroy(struct chunk *c)
{
	fstack_destroy(&c->chunk);
	fstack_destroy(&c->chunk_line_info);

	for (size_t i = 0; i < fstack_len(&c->constants); i++) {
		struct val v;
		v = *(struct val *) fstack_at(&c->constants, i);

		if (IS_OBJ(v) && AS_OBJ(v) != NULL)
			obj_free(AS_OBJ(v));
	}

	fstack_destroy(&c->constants);
}

static void chunk_xwrite(struct chunk *c, int line, const char *chunk, size_t len)
{
	if (!len)
		return;

	/* TODO: multipush */
	for (size_t i = 0; i < len; i++)
		fstack_xpush(&c->chunk, &chunk[i]);

	bool count_incremented = false;

	if (fstack_len(&c->chunk_line_info) > 0) {
		struct chunk_line_info *cli = fstack_top(&c->chunk_line_info);

		if (cli->line == line) {
			cli->count += len;
			count_incremented = true;
		}
	}

	if (!count_incremented)
		fstack_xpush(&c->chunk_line_info,
			     &(struct chunk_line_info) {
				.line = line,
				.count = len
			     });
}

void chunk_xwrite_inst(struct chunk *c, int line, struct inst inst)
{
	chunk_xwrite(c, line, &(char) { inst.op }, 1);
	size_t arg_len = inst_arg_len(inst.op);

	if (inst.args != NULL) {
		chunk_xwrite(c, line, inst.args, arg_len);
	} else {
		char dummy[arg_len];
		chunk_xwrite(c, line, dummy, arg_len);
	}
}

void chunk_overwrite_inst(struct chunk *c, size_t offset, struct inst inst)
{
	*(uint8_t *) fstack_at(&c->chunk, offset) = inst.op;

	if (inst.args != NULL) {
		for (size_t i = 0; i < inst_arg_len(inst.op); i++)
			*(uint8_t *) fstack_at(&c->chunk, offset + i + 1) =
				((uint8_t *) inst.args)[i];
	}
}

size_t chunk_len(const struct chunk *c)
{
	return fstack_len(&c->chunk);
}


void chunk_disassemble(struct chunk *c, FILE *out, size_t offset, size_t len)
{
	if (fstack_len(&c->chunk) == 0)
		return;

	size_t cli_index = 0;
	size_t total_instructions = 0;
	int prev_line = -1;
	int line = -2;  /* no need to initialization, but due to false-positive
			 * uninitialized variable warning I set to -2 */

	for (size_t i = 0;
	     (len == 0 || i < len) && offset + i < fstack_len(&c->chunk);) {
		for (; total_instructions <= offset + i; cli_index++) {
			struct chunk_line_info *cli =
				fstack_at(&c->chunk_line_info, cli_index);

			total_instructions += cli->count;
			line = cli->line;
		}

		struct inst ins = chunk_read_inst(c, offset + i);
		fprintf(out, "%04zu ", offset + i);
		i += 1 + inst_arg_len(ins.op);

		inst_print(ins, out, prev_line == line ? -1 : line);

		prev_line = line;
	}
}

struct inst chunk_read_inst(const struct chunk *c, size_t offset)
{
	char *inst_bytes = fstack_at((struct fstack *) &c->chunk, offset);

	return (struct inst) {
		.op = (enum opcode) inst_bytes[0],
		.args = inst_bytes + 1
	};
}

uint32_t chunk_xpush_constant(struct chunk *c, const struct val *v)
{
	uint32_t constant_id = fstack_len(&c->constants);

	fstack_xpush(&c->constants, v);

	return constant_id;
}
