#include "../include/common.h"
#include "../include/instructions.h"
#include "../include/string_pool.h"

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


size_t inst_arg_len(enum opcode op)
{
	switch (op) {
	case OP_CONSTANT_LONG:
	case OP_DEFINE_GLOBAL_LONG:
	case OP_GET_GLOBAL_LONG:
	case OP_SET_GLOBAL_LONG:
	case OP_GET_LOCAL_LONG:
	case OP_SET_LOCAL_LONG:
	case OP_GET_UPVALUE_LONG:
	case OP_SET_UPVALUE_LONG:
	case OP_JUMP:
	case OP_JUMP_IF_FALSE:
	case OP_JUMP_BACK:
	case OP_CLOSURE_LONG:
		return 3;

	case OP_CONSTANT:
	case OP_DEFINE_GLOBAL:
	case OP_GET_GLOBAL:
	case OP_SET_GLOBAL:
	case OP_GET_LOCAL:
	case OP_SET_LOCAL:
	case OP_GET_UPVALUE:
	case OP_SET_UPVALUE:
	case OP_CLOSURE:
	case OP_CALL:
		return 1;

	default:
		return 0;
	}
}

size_t inst_len(struct inst inst)
{
	size_t len = inst_arg_len(inst.op) + 1;

	if (inst.op == OP_CLOSURE || inst.op == OP_CLOSURE_LONG) {
		size_t offset = inst_arg_len(inst.op);

		int arg_len;
		while ((arg_len = inst_get_next_closure_arg(inst,
							    offset,
							    NULL,
							    NULL))) {
			len += arg_len + 1;
			offset += arg_len + 1;
		}

		len++;
	}

	return len;
}


static void print_ident(uint32_t ident_id,
			FILE *out,
			const struct str_pool *idents)
{
	const char *out_chars;
	size_t out_len;

	clox_assert(str_pool_get_chars(idents,
				       ident_id,
				       &out_chars,
				       &out_len),
		    "ident not found, possibly GC'ed!");

	fprintf(out, "%.*s", (int) out_len, out_chars);
}

void inst_print(struct inst inst,
		FILE *out,
		int line,
		const struct str_pool *idents)
{
	bool first = true;
#define LINE do { \
		if (line != -1 && first) \
			fprintf(out, "\t%4d ", line); \
		else \
			fprintf(out, "\t   | "); \
		if (first) { \
			const char *const opcode_name = \
				inst.op > LAST_OPCODE ? "??" : opcode_names[inst.op]; \
			fprintf(out, "%16s . ", opcode_name); \
		} else { \
			fprintf(out, "%16s | ", ""); \
		} \
		first = false; \
	} while (0)

	LINE;

	switch (inst.op) {
	case OP_CONSTANT_LONG:
	case OP_JUMP:
	case OP_JUMP_IF_FALSE:
	case OP_JUMP_BACK:
	case OP_CLOSURE_LONG:
	case OP_GET_LOCAL_LONG:
	case OP_SET_LOCAL_LONG:

	case OP_CONSTANT:
	case OP_GET_UPVALUE:
	case OP_SET_UPVALUE:
	case OP_CLOSURE:
	case OP_CALL:
	case OP_GET_LOCAL:
	case OP_SET_LOCAL:
		fprintf(out, "%"PRIu32, inst_get_u8_or_u24_arg(inst, 0));
		break;

	case OP_DEFINE_GLOBAL_LONG:
	case OP_GET_GLOBAL_LONG:
	case OP_SET_GLOBAL_LONG:

	case OP_DEFINE_GLOBAL:
	case OP_GET_GLOBAL:
	case OP_SET_GLOBAL: {
		uint32_t ident_id = inst_get_u8_or_u24_arg(inst, 0);
		fprintf(out, "%"PRIu32" '", ident_id);
		print_ident(ident_id, out, idents);
		fprintf(out, "'");
		break;
	}

	default:
		break;
	}

	if (inst.op == OP_CLOSURE || inst.op == OP_CLOSURE_LONG) {
		int arg_len;
		size_t offset = inst_arg_len(inst.op);

		uint32_t out_index;
		bool out_is_local;

		fprintf(out, " <fn>");
		while ((arg_len = inst_get_next_closure_arg(inst,
							    offset,
							    &out_index,
							    &out_is_local))) {
			fputc('\n', out);
			LINE;
			fprintf(out,
				" %s %"PRIu32 "",
				out_is_local ? "(local)" : "(upvalue)",
				out_index);

			offset += arg_len + 1;
		}
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

int inst_get_next_closure_arg(struct inst inst,
			      size_t offset,
			      uint32_t *out_index,
			      bool *out_is_local)
{
	union op_closure_arg *arg =
		(union op_closure_arg *) &((uint8_t *) inst.args)[offset];

	if (arg->last == 1)
		return 0;

	if (out_is_local)
		*out_is_local = arg->upvalue.is_local;

	if (arg->upvalue.is_long) {
		if (out_index)
			*out_index = inst_get_u24_arg(inst, offset + 1);

		return 3;
	} else {
		if (out_index)
			*out_index = inst_get_u8_arg(inst, offset + 1);

		return 1;
	}
}
