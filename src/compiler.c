#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/instructions.h"
#include "../include/value.h"

#include "../vendor/rdesc/include/cst_macros.h"
#include "../vendor/rdesc/include/util.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


#define emit_byte(opcode) \
	chunk_xwrite(c, *line, (const char *) &(uint8_t) { opcode }, 1)

#define emit_bytes(opcode, opcode2) do { \
		emit_byte(opcode); emit_byte(opcode2); \
	} while (0)

#define emit_u8_or_u24(num) do { \
		chunk_xwrite(c, *line, \
			     (const char *) (num < 256 ? \
						     &(uint8_t) { (uint8_t) num } : \
						     &*(uint8_t[3]) { \
							num & 255, \
							(num >> 8) & 255, \
							(num >> 16) & 255, \
						     }), \
			     num < 256 ? 1 : 3); \
	} while (0)

#define emit_const(v) do { \
		uint32_t constant_id = chunk_xpush_constant(c, &v); \
		emit_byte(constant_id < 256 ? OP_CONSTANT : OP_CONSTANT_LONG); \
		emit_u8_or_u24(constant_id); \
	} while (0)

#define update_line(tk) do { \
		*line = ((struct seminfo *) rseminfo(tk))->line; \
	} while (0)


#define SEMINFO(n) (((struct seminfo *) rseminfo(n))->seminfo)

#define SEMINFO_NUM(n) (SEMINFO(n).num)

#define SEMINFO_STR(n) (SEMINFO(n).str)

#define SEMINFO_IDENT_ID(n) (SEMINFO(n).ident_id)

static void compile_expression(struct chunk *c,
			       struct rdesc_node n,
			       int *line,
			       bool is_lvalue)
{
	switch (rid(n)) {
	/* the rrr rules that have a rrr child */
	case NT_LOGIC_OR:
	case NT_LOGIC_AND:
	case NT_EQUALITY:
	case NT_COMPARISON:
	case NT_TERM:
		switch (ralt_idx(n)) {
		case 0:
			if (is_lvalue)/* TODO: rvalue error handling */
				clox_fatal("expression is not assignable");

			rdesc_flip_left(n, 2);
			compile_expression(c, rchild(n, 0), line, is_lvalue);
			compile_expression(c, rchild(n, 2), line, is_lvalue);

			break;

		case 1:
			rdesc_flip_left(n, 0);
			compile_expression(c, rchild(n, 0), line, is_lvalue);

			break;
		}
		break;

	default:
		break;
	}

	switch (rid(n)) {
	case NT_EXPRESSION:
		compile_expression(c, rchild(n, 0), line, is_lvalue);
		break;

	case NT_ASGN:
		rdesc_flip_left(n, 0);

		is_lvalue = ralt_idx(rchild(n, 1)) == 0;

		compile_expression(c, rchild(n, 1), line, false);
		compile_expression(c, rchild(n, 0), line, is_lvalue);

		break;

	case NT_ASGN_OPTEQ:
		if (ralt_idx(n) == 0)
			compile_expression(c, rchild(n, 1), line, is_lvalue);
		break;

	case NT_LOGIC_OR:
		if (ralt_idx(n) == 0)
			emit_byte(OP_OR);
		break;

	case NT_LOGIC_AND:
		if (ralt_idx(n) == 0)
			emit_byte(OP_AND);
		break;

	case NT_EQUALITY:
		if (ralt_idx(n) == 0) {
			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_bytes(OP_EQUAL, OP_NOT);
				break;
			case 1:
				emit_byte(OP_EQUAL);
				break;
			}
		}
		break;

	case NT_COMPARISON:
		if (ralt_idx(n) == 0) {
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
		}
		break;

	case NT_TERM:
		if (ralt_idx(n) == 0) {
			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_byte(OP_SUBSTRACT);
				break;
			case 1:
				emit_byte(OP_ADD);
				break;
			}
		}
		break;

	case NT_FACTOR:
		switch (ralt_idx(n)) {
		case 0:
			if (is_lvalue)/* TODO: rvalue error handling */
				clox_fatal("expression is not assignable");

			compile_expression(c, rchild(n, 0), line, is_lvalue);
			compile_expression(c, rchild(n, 2), line, is_lvalue);

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
			compile_expression(c, rchild(n, 0), line, is_lvalue);

			break;
		}
		break;

	case NT_UNARY:
		switch (ralt_idx(n)) {
		case 0:
			if (is_lvalue)/* TODO: rvalue error handling */
				clox_fatal("expression is not assignable");

			compile_expression(c, rchild(n, 1), line, is_lvalue);

			if (ralt_idx(rchild(n, 0)) == 1)
				emit_byte(OP_NEGATE);
			else if (ralt_idx(rchild(n, 0)) == 2)
				emit_byte(OP_NOT);

			break;

		case 1:
			compile_expression(c, rchild(n, 0), line, is_lvalue);

			break;
		}
		break;

	case NT_CALL:
		compile_expression(c, rchild(n, 0), line, is_lvalue);
		compile_expression(c, rchild(n, 1), line, is_lvalue);
		break;

	case NT_CALL_OPTARGS_OR_GETATTR:
		/* TODO: catch non-assignable, i.e. a() but not a().c.
		 * rvalue if ends with a call */
		switch (ralt_idx(n)) {
		case 0:
			clox_fatal("function calls are not implemented yet");
			break;

		case 1:
			clox_fatal("getattr is not implemented yet");
			break;

		case 2:
			break;
		}
		break;

	case NT_PRIMARY:
		if (is_lvalue && ralt_idx(n) < 6) /* TODO: rvalue error handling */
			clox_fatal("expression is not assignable");

		switch (ralt_idx(n)) {
		case 0:
			emit_const(NUM_VAL(SEMINFO_NUM(rchild(n, 0))));
			break;

		case 1: {
			char *chars = SEMINFO_STR(rchild(n, 0));
			struct obj_string *obj = obj_string_new(chars,
								strlen(chars));

			emit_const(OBJ_VAL((struct obj *) obj));
			break;
		}

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
			compile_expression(c, rchild(n, 1), line, is_lvalue);
			break;

		case 6:
			clox_fatal("'this' keyword is not implemented yet");
			break;

		case 7:
			clox_fatal("attr inheritance is not implemented yet");
			break;

		case 8: {
			uint32_t ident_id = SEMINFO_IDENT_ID(rchild(n, 0));

			if (is_lvalue)
				emit_byte(ident_id < 256 ?
						OP_SET_GLOBAL : OP_SET_GLOBAL_LONG);
			else
				emit_byte(ident_id < 256 ?
						OP_GET_GLOBAL : OP_GET_GLOBAL_LONG);

			emit_u8_or_u24(ident_id);
			break;
		}
		}
		break;
	}
}

static void compile_var_decl(struct chunk *c, struct rdesc_node n, int *line)
{
	update_line(rchild(n, 0));
	int ident_id = SEMINFO_IDENT_ID(rchild(n, 1));

	struct rdesc_node optasgn = rchild(n, 2);
	if (ralt_idx(optasgn) == 0) {
		compile_expression(c, rchild(optasgn, 1), line, false);
	} else {
		emit_byte(OP_NIL);
	}

	emit_byte(ident_id < 256 ? OP_DEFINE_GLOBAL : OP_DEFINE_GLOBAL_LONG);
	emit_u8_or_u24(ident_id);
}

static void compile_stmt(struct chunk *c, struct rdesc_node n, int *line)
{
	n = rchild(n, 0);

	switch (rid(n)) {
	case NT_EXPR_STMT:
		/* <expr> ; */
		update_line(rchild(n, 1));
		compile_expression(c, rchild(n, 0), line, false);
		emit_byte(OP_POP);  /* discard the expression result */
		break;

	case NT_FOR_STMT:
		clox_fatal("for_stmt is not implemented yet");
		break;

	case NT_IF_STMT:
		clox_fatal("if_stmt is not implemented yet");
		break;

	case NT_PRINT_STMT:
		/* print <expr> ; */
		update_line(rchild(n, 0));
		compile_expression(c, rchild(n, 1), line, false);
		emit_byte(OP_PRINT);
		update_line(rchild(n, 2));
		break;

	case NT_RETURN_STMT:
		clox_fatal("return_stmt is not implemented yet");
		break;

	case NT_WHILE_STMT:
		clox_fatal("while_stmt is not implemented yet");
		break;

	case NT_BLOCK:
		clox_fatal("statement blocks are not implemented yet");
		break;
	}
}

static void compile_decl(struct chunk *c, struct rdesc_node n, int *line)
{
	switch (ralt_idx(n)) {
	case 0:
		clox_fatal("classes are not implemented yet");
		break;

	case 1:
		clox_fatal("functions are not implemented yet");
		break;

	case 2:
		compile_var_decl(c, rchild(n, 0), line);
		break;

	case 3:
		compile_stmt(c, rchild(n, 0), line);
		break;
	}

	emit_byte(OP_RETURN);
}
/* shall procide NT_DECL */
void chunk_xcompile(struct chunk *c, struct rdesc_node n)
{
	int line = 0;

	compile_decl(c, n, &line);
}
