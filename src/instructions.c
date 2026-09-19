#include "../include/instructions.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


size_t inst_arg_len(enum opcode op)
{
	switch (op) {
	case OP_CONSTANT_LONG:
	case OP_CONSTANT_ONCE_LONG:
	case OP_DEFINE_GLOBAL_LONG:
	case OP_GET_GLOBAL_LONG:
	case OP_SET_GLOBAL_LONG:
	case OP_GET_LOCAL_LONG:
	case OP_SET_LOCAL_LONG:
	case OP_JUMP:
	case OP_JUMP_IF_FALSE:
	case OP_JUMP_BACK:
		return 3;

	case OP_CONSTANT:
	case OP_CONSTANT_ONCE:
	case OP_DEFINE_GLOBAL:
	case OP_GET_GLOBAL:
	case OP_SET_GLOBAL:
	case OP_GET_LOCAL:
	case OP_SET_LOCAL:
	case OP_CALL:
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
	case OP_DEFINE_GLOBAL_LONG:
	case OP_GET_GLOBAL_LONG:
	case OP_SET_GLOBAL_LONG:
	case OP_GET_LOCAL_LONG:
	case OP_SET_LOCAL_LONG:
	case OP_JUMP:
	case OP_JUMP_IF_FALSE:
	case OP_JUMP_BACK:
		fprintf(out, "%"PRIu32, inst_get_u24_arg(inst, 0));
		break;

	case OP_CONSTANT:
	case OP_DEFINE_GLOBAL:
	case OP_GET_GLOBAL:
	case OP_SET_GLOBAL:
	case OP_GET_LOCAL:
	case OP_SET_LOCAL:
		fprintf(out, "%d", inst_get_u8_arg(inst, 0));
		break;

	default:
		break;
	}

	fprintf(out, "\n");
}

#define argbyte(n) (((uint8_t *) inst.args)[n])

uint8_t inst_get_u8_arg(struct inst inst, size_t offset)
{
	return argbyte(offset);
}

uint32_t inst_get_u24_arg(struct inst inst, size_t offset)
{
	return (uint32_t) (argbyte(offset + 2) << 16) |
		(uint32_t) (argbyte(offset + 1) << 8) |
		(uint32_t) argbyte(offset);
}
