#include "../include/instructions.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


size_t inst_arg_len(enum opcode op)
{
	switch (op) {
	case OP_CONSTANT_LONG:
		return 3;

	case OP_CONSTANT:
		return 1;

	default:
		return 0;
	}
}

void inst_print(struct inst inst, FILE *out, int line)
{
	if (line == -1)
		fprintf(out, "   | ");
	else
		fprintf(out, "%4d ", line);

	const char *const opcode_name =
		inst.op > LAST_OPCODE ? "??" : opcode_names[inst.op];

	fprintf(out, "%16s . ", opcode_name);

	switch (inst.op) {
	case OP_CONSTANT_LONG:
		fprintf(out, "%"PRIu32, inst_get_u24_arg(inst, 0));
		break;

	case OP_CONSTANT:
		fprintf(out, "%d", inst_get_u8_arg(inst, 0));
		break;

	default:
		break;
	}

	fprintf(out, "\n");
}

#define argbyte(n) (((char *) inst.args)[n])

uint8_t inst_get_u8_arg(struct inst inst, size_t offset)
{
	return argbyte(offset);
}

uint32_t inst_get_u24_arg(struct inst inst, size_t offset)
{
	return (uint32_t) argbyte(offset + 2) << 16 |
		(uint32_t) argbyte(offset + 1) << 8 |
		(uint32_t) argbyte(offset);
}
