#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/instructions.h"
#include "../include/value.h"

#include "../vendor/libfun/include/stack.h"

#include "../vendor/rdesc/include/cst_macros.h"
#include "../vendor/rdesc/include/util.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>


#define emit_byte(opcode) \
	chunk_xwrite(c, current->line, (const char *) &(uint8_t) { opcode }, 1)

#define emit_bytes(opcode, opcode2) do { \
		emit_byte(opcode); emit_byte(opcode2); \
	} while (0)

#define u32_to_vm_u24(num) ((uint8_t[3]) { \
				(num) & 255, \
				((num) >> 8) & 255, \
				((num) >> 16) & 255, \
			   })

#define emit_u8_or_u24(num) do { \
		chunk_xwrite(c, current->line, \
			     (const char *) (num < 256 ? \
						     &(uint8_t) { (uint8_t) num } : \
						     &*u32_to_vm_u24(num)), \
			     num < 256 ? 1 : 3); \
	} while (0)

#define emit_const(v) do { \
		uint32_t constant_id = chunk_xpush_constant(c, &v); \
		emit_byte(constant_id < 256 ? OP_CONSTANT : OP_CONSTANT_LONG); \
		emit_u8_or_u24(constant_id); \
	} while (0)

#define update_line(tk) do { \
		current->line = ((struct seminfo *) rseminfo(tk))->line; \
	} while (0)


#define SEMINFO(n) (((struct seminfo *) rseminfo(n))->seminfo)

#define SEMINFO_NUM(n) (SEMINFO(n).num)

#define SEMINFO_STR(n) (SEMINFO(n).str)

#define SEMINFO_IDENT_ID(n) (SEMINFO(n).ident_id)


struct compiler {
	int line;
	struct fstack locals;
	int scope_depth;
};

struct local {
	uint32_t ident_id;
	int depth;
};


static void compile_decl(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_stmt(struct chunk *, struct rdesc_node, struct compiler *);

static uint32_t resolve_local(struct compiler *current, uint32_t ident_id)
{
	for (size_t i = fstack_len(&current->locals); i > 0; i--) {
		struct local *var = fstack_at(&current->locals, i - 1);
		if (var->ident_id == ident_id)
			return i - 1;
	}

	return UINT_MAX;
}

static void compile_expression(struct chunk *c,
			       struct rdesc_node n,
			       struct compiler *current,
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
			compile_expression(c, rchild(n, 0), current, is_lvalue);
			compile_expression(c, rchild(n, 2), current, is_lvalue);

			break;

		case 1:
			rdesc_flip_left(n, 0);
			compile_expression(c, rchild(n, 0), current, is_lvalue);

			break;
		}
		break;

	default:
		break;
	}

	switch (rid(n)) {
	case NT_EXPRESSION:
		compile_expression(c, rchild(n, 0), current, is_lvalue);
		break;

	case NT_ASGN:
		rdesc_flip_left(n, 0);

		is_lvalue = ralt_idx(rchild(n, 1)) == 0;

		compile_expression(c, rchild(n, 1), current, false);
		compile_expression(c, rchild(n, 0), current, is_lvalue);

		break;

	case NT_ASGN_OPTEQ:
		if (ralt_idx(n) == 0)
			compile_expression(c, rchild(n, 1), current, is_lvalue);
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

			compile_expression(c, rchild(n, 0), current, is_lvalue);
			compile_expression(c, rchild(n, 2), current, is_lvalue);

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
			compile_expression(c, rchild(n, 0), current, is_lvalue);

			break;
		}
		break;

	case NT_UNARY:
		switch (ralt_idx(n)) {
		case 0:
			if (is_lvalue)/* TODO: rvalue error handling */
				clox_fatal("expression is not assignable");

			compile_expression(c, rchild(n, 1), current, is_lvalue);

			if (ralt_idx(rchild(n, 0)) == 1)
				emit_byte(OP_NEGATE);
			else if (ralt_idx(rchild(n, 0)) == 2)
				emit_byte(OP_NOT);

			break;

		case 1:
			compile_expression(c, rchild(n, 0), current, is_lvalue);

			break;
		}
		break;

	case NT_CALL:
		compile_expression(c, rchild(n, 0), current, is_lvalue);
		compile_expression(c, rchild(n, 1), current, is_lvalue);
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
			compile_expression(c, rchild(n, 1), current, is_lvalue);
			break;

		case 6:
			clox_fatal("'this' keyword is not implemented yet");
			break;

		case 7:
			clox_fatal("attr inheritance is not implemented yet");
			break;

		case 8: {
			uint32_t ident_id = SEMINFO_IDENT_ID(rchild(n, 0));
			uint32_t local_slot = resolve_local(current, ident_id);

			if (local_slot == UINT_MAX) {
				if (is_lvalue)
					emit_byte(ident_id < 256 ?
							OP_SET_GLOBAL :
							OP_SET_GLOBAL_LONG);
				else
					emit_byte(ident_id < 256 ?
							OP_GET_GLOBAL :
							OP_GET_GLOBAL_LONG);

				emit_u8_or_u24(ident_id);
			} else {
				if (is_lvalue)
					emit_byte(ident_id < 256 ?
							OP_SET_LOCAL :
							OP_SET_LOCAL_LONG);
				else
					emit_byte(ident_id < 256 ?
							OP_GET_LOCAL :
							OP_GET_LOCAL_LONG);

				emit_u8_or_u24(local_slot);
			}
			break;
		}
		}
		break;
	}
}

static void compile_var_decl(struct chunk *c,
			     struct rdesc_node n,
			     struct compiler *current)
{
	update_line(rchild(n, 0));
	uint32_t ident_id = SEMINFO_IDENT_ID(rchild(n, 1));

	struct rdesc_node optasgn = rchild(n, 2);
	if (ralt_idx(optasgn) == 0) {
		compile_expression(c, rchild(optasgn, 1), current, false);
	} else {
		emit_byte(OP_NIL);
	}

	uint32_t local_slot = resolve_local(current, ident_id);
	if (local_slot == UINT16_MAX || current->scope_depth == 0) {
		emit_byte(ident_id < 256 ?
				OP_DEFINE_GLOBAL : OP_DEFINE_GLOBAL_LONG);
		emit_u8_or_u24(ident_id);
	} else {
		fstack_xpush(&current->locals,
			     &(struct local) {
				     .ident_id = ident_id,
				     .depth = current->scope_depth
			     });
	}
}

static void compile_block(struct chunk *c,
			  struct rdesc_node n,
			  struct compiler *current)
{
	current->scope_depth++;

	do {
		compile_decl(c, rchild(n, 0), current);

		n = rchild(n, 1);
	} while (ralt_idx(n) == 0);

	/* pop local variables */
	struct local *local;
	while (fstack_len(&current->locals) > 0 &&
	       (local = fstack_top(&current->locals)) &&
	       local->depth == current->scope_depth) {
		fstack_pop(&current->locals);
		emit_byte(OP_POP);
	}

	current->scope_depth--;
}

static void compile_if_stmt(struct chunk *c,
			    struct rdesc_node n,
			    struct compiler *current)
{
	compile_expression(c, rchild(n, 2), current, false);

	chunk_xwrite_inst(c, current->line, (struct inst) { .op = OP_JUMP_IF_FALSE, .args = NULL });
	size_t if_inst = chunk_len(c) - inst_arg_len(OP_JUMP_IF_FALSE);

	size_t then_start = chunk_len(c);

	emit_byte(OP_POP);
	compile_stmt(c, rchild(n, 4), current);

	chunk_xwrite_inst(c, current->line, (struct inst) { .op = OP_JUMP, .args = NULL });
	size_t else_inst = chunk_len(c) - inst_arg_len(OP_JUMP);

	size_t else_start = chunk_len(c);

	emit_byte(OP_POP);
	size_t then_end = chunk_len(c);

	struct rdesc_node else_n = rchild(n, 5);
	if (ralt_idx(else_n) == 0) {
		update_line(rchild(else_n, 0));
		compile_stmt(c, rchild(else_n, 1), current);
	}

	size_t else_end = chunk_len(c);

	chunk_override_inst(c, if_inst,
			    (struct inst) {
				.op = OP_JUMP_IF_FALSE,
				.args = &*u32_to_vm_u24(then_end - then_start - 1)
			    });

	chunk_override_inst(c, else_inst,
			    (struct inst) {
				.op = OP_JUMP,
				.args = &*u32_to_vm_u24(else_end - else_start)
			    });
}

static void compile_for_stmt(struct chunk *c,
			     struct rdesc_node n,
			     struct compiler *current)
{
}

static void compile_stmt(struct chunk *c,
			 struct rdesc_node n,
			 struct compiler *current)
{
	n = rchild(n, 0);

	switch (rid(n)) {
	case NT_EXPR_STMT:
		/* <expr> ; */
		update_line(rchild(n, 1));
		compile_expression(c, rchild(n, 0), current, false);
		emit_byte(OP_POP);  /* discard the expression result */
		break;

	case NT_FOR_STMT:
		clox_fatal("for_stmt is not implemented yet");
		break;

	case NT_IF_STMT:
		/* if ( <expr> ) <stmt> <if_optelse_stmt> */
		update_line(rchild(n, 0));
		compile_if_stmt(c, n, current);
		break;

	case NT_PRINT_STMT:
		/* print <expr> ; */
		update_line(rchild(n, 0));
		compile_expression(c, rchild(n, 1), current, false);
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
		compile_block(c, rchild(n, 1), current);
		break;
	}
}

static void compile_decl(struct chunk *c,
			 struct rdesc_node n,
			 struct compiler *current)
{
	switch (ralt_idx(n)) {
	case 0:
		clox_fatal("classes are not implemented yet");
		break;

	case 1:
		clox_fatal("functions are not implemented yet");
		break;

	case 2:
		compile_var_decl(c, rchild(n, 0), current);
		break;

	case 3:
		compile_stmt(c, rchild(n, 0), current);
		break;
	}
}

/* shall procide NT_DECL */
void chunk_xcompile(struct chunk *c, struct rdesc_node n)
{
	struct compiler current;

	current.line = 0;
	current.scope_depth = 0;
	fstack_xinit(&current.locals, sizeof(struct local));

	compile_decl(c, n, &current);

	chunk_xwrite_inst(c, current.line, (struct inst) { .op = OP_RETURN });

	clox_assert(fstack_len(&current.locals) == 0,
		    "local stack should have 0 length");

	fstack_destroy(&current.locals);
}
