#include "../include/chunk.h"
#include "../include/clox.h"
#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/instructions.h"

#include "../vendor/rdesc/include/cst_macros.h"
#include "../vendor/rdesc/include/util.h"

#include <stdint.h>


#define emit_byte(opcode) \
	chunk_xwrite_inst(c, *line, (struct inst) { .op = opcode })

#define emit_bytes(opcode, opcode2) do { \
		emit_byte(opcode); emit_byte(opcode2); \
	} while (0)

#define update_line(tk) do { \
		*line = ((struct seminfo *) rseminfo(tk))->line; \
	} while (0)

#define emit_op_const(v) do { \
		uint32_t constant_id = chunk_xpush_constant(c, &v); \
		chunk_xwrite_inst(c, *line, \
				 (struct inst) { \
					.op = constant_id > 255 ? \
						OP_CONSTANT_LONG : OP_CONSTANT, \
					.args = constant_id < 255 ? \
						&(uint8_t) { constant_id } : \
						&*(uint8_t[]) { \
							constant_id & 255, \
							(constant_id >> 8) & 255, \
							(constant_id >> 16) & 255, \
						} \
				 }); \
	} while (0)

static void compile_expression(struct chunk *c, struct rdesc_node n, int *line)
{
	switch (rid(n)) {
	case NT_EXPRESSION:
		rdesc_flip_left(n, 0);

		compile_expression(c, rchild(n, 0), line);
		break;

	case NT_EQUALITY:
		switch (ralt_idx(n)) {
		case 0:
			rdesc_flip_left(n, 2);

			compile_expression(c, rchild(n, 0), line);
			compile_expression(c, rchild(n, 2), line);

			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_bytes(OP_EQUAL, OP_NOT);
				break;
			case 1:
				emit_byte(OP_EQUAL);
				break;
			}

			break;

		case 1:
			rdesc_flip_left(n, 0);

			compile_expression(c, rchild(n, 0), line);

			break;
		}
		break;

	case NT_COMPARISON:
		switch (ralt_idx(n)) {
		case 0:
			rdesc_flip_left(n, 2);

			compile_expression(c, rchild(n, 0), line);
			compile_expression(c, rchild(n, 2), line);

			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_byte(OP_GREATER);
				break;
			case 1:
				emit_bytes(OP_LESS, OP_NOT);
				break;
			case 2:
				emit_byte(OP_LESS);
				break;
			case 3:
				emit_bytes(OP_GREATER, OP_NOT);
				break;
			}

			break;

		case 1:
			rdesc_flip_left(n, 0);

			compile_expression(c, rchild(n, 0), line);

			break;
		}
		break;

	case NT_TERM:
		switch (ralt_idx(n)) {
		case 0:
			rdesc_flip_left(n, 2);

			compile_expression(c, rchild(n, 0), line);
			compile_expression(c, rchild(n, 2), line);

			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_byte(OP_SUBSTRACT);
				break;
			case 1:
				emit_byte(OP_ADD);
				break;
			}

			break;

		case 1:
			rdesc_flip_left(n, 0);

			compile_expression(c, rchild(n, 0), line);

			break;
		}
		break;

	case NT_FACTOR:
		switch (ralt_idx(n)) {
		case 0:
			compile_expression(c, rchild(n, 0), line);
			compile_expression(c, rchild(n, 2), line);

			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_byte(OP_DIVIDE);
				break;
			case 1:
				emit_byte(OP_MULTIPLY);
				break;
			}

			break;

		case 1:
			compile_expression(c, rchild(n, 0), line);

			break;
		}
		break;

	case NT_UNARY:
		switch (ralt_idx(n)) {
		case 0:
			compile_expression(c, rchild(n, 1), line);

			if (ralt_idx(rchild(n, 0)) == 1)
				emit_byte(OP_NEGATE);

			break;

		case 1:
			compile_expression(c, rchild(n, 0), line);
			break;
		}
		break;

	case NT_PRIMARY:
		switch (ralt_idx(n)) {
		case 0:
			emit_op_const(NUM_VAL(((struct seminfo *)
					rseminfo(rchild(n, 0)))->seminfo.num));
			break;

		case 1:
			clox_fatal("srings are not implemented yet");
			/*
			emit_op_const(STR_VAL(((struct seminfo *)
					rseminfo(rchild(n, 0)))->seminfo.str));
			*/
			break;

		case 2:
			emit_byte(OP_TRUE);
			break;

		case 3:
			emit_byte(OP_FALSE);
			break;

		case 4:
			emit_byte(OP_NIL);
			break;

		case 5:
			compile_expression(c, rchild(n, 1), line);
			break;
		}
		break;
	}
}

static void compile_stmt(struct chunk *c, struct rdesc_node n, int *line)
{
	switch (ralt_idx(n)) {
	case 0:
		update_line(rchild(n, 1));
		compile_expression(c, rchild(n, 0), line);
		break;

	case 1:
		update_line(rchild(n, 0));
		compile_expression(c, rchild(n, 1), line);
		emit_byte(OP_PRINT);
		update_line(rchild(n, 2));
		break;
	}

	emit_byte(OP_RETURN);
}

/* shall procide NT_DECL */
void chunk_xcompile(struct chunk *c, struct rdesc_node n)
{
	int line = 0;

	switch (ralt_idx(n)) {
	case 0:
		compile_stmt(c, rchild(n, 0), &line);
		break;
	}
}
