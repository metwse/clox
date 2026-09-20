#include "compiler_internal.h"

#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/instructions.h"
#include "../include/grammar.h"
#include "../include/object.h"
#include "../include/value.h"

#include "../vendor/rdesc/include/cst_macros.h"
#include "../vendor/rdesc/include/util.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#define SEMINFO(n) (((struct seminfo *) rseminfo(n))->seminfo)

#define SEMINFO_NUM(n) (SEMINFO(n).num)

#define SEMINFO_STR(n) (SEMINFO(n).str)

#define SEMINFO_IDENT_ID(n) (SEMINFO(n).ident_id)


static void compile_expression(struct chunk *, struct rdesc_node, struct compiler *, bool);
static void compile_args(struct chunk *, struct rdesc_node, struct compiler *, uint32_t *);
static void compile_block(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_if_stmt(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_for_stmt(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_while_stmt(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_stmt(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_var_decl(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_function_decl(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_return_stmt(struct chunk *, struct rdesc_node, struct compiler *);
static void compile_decl(struct chunk *, struct rdesc_node, struct compiler *);


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
			emit_inst(OP_OR);
		break;

	case NT_LOGIC_AND:
		if (ralt_idx(n) == 0)
			emit_inst(OP_AND);
		break;

	case NT_EQUALITY:
		if (ralt_idx(n) == 0) {
			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_insts(OP_EQUAL, OP_NOT);
				break;
			case 1:
				emit_inst(OP_EQUAL);
				break;
			}
		}
		break;

	case NT_COMPARISON:
		if (ralt_idx(n) == 0) {
			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_inst(OP_GREATER);
				break;
			case 1:
				emit_insts(OP_LESS, OP_NOT);
				break;
			case 2:
				emit_inst(OP_LESS);
				break;
			case 3:
				emit_insts(OP_GREATER, OP_NOT);
				break;
			}
		}
		break;

	case NT_TERM:
		if (ralt_idx(n) == 0) {
			switch (ralt_idx(rchild(n, 1))) {
			case 0:
				emit_inst(OP_SUBSTRACT);
				break;
			case 1:
				emit_inst(OP_ADD);
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
				emit_inst(OP_DIVIDE);
				break;
			case 1:
				emit_inst(OP_MULTIPLY);
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
				emit_inst(OP_NEGATE);
			else if (ralt_idx(rchild(n, 0)) == 2)
				emit_inst(OP_NOT);

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
		switch (ralt_idx(n)) {
		case 0: {
			if (is_lvalue) /* TODO: rvalue error handling */
				clox_fatal("expression is not assignable");

			uint32_t arg_count = 0;

			struct rdesc_node optargs = rchild(n, 1);
			if (ralt_idx(optargs) == 0)
				compile_args(c,
					     rchild(optargs, 0),
					     current,
					     &arg_count);

			if (arg_count > 255)
				clox_fatal("too many arguments!");

			emit_inst_u8(OP_CALL, arg_count);

			compile_expression(c, rchild(n, 3), current, is_lvalue);
			break;
		}

		case 1:
			/* TODO: catch non-assignable, i.e. a() but not a().c.
			 * rvalue if ends with a call */

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
			emit_inst(OP_TRUE);
			break;

		case 3:
			emit_inst(OP_FALSE);
			break;

		case 4:
			emit_inst(OP_NIL);
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

			if (is_lvalue)
				emit_inst_set(ident_id);
			else
				emit_inst_get(ident_id);
			break;
		}
		}
		break;
	}
}

static void compile_args(struct chunk *c,
			 struct rdesc_node n,
			 struct compiler *current,
			 uint32_t *arg_count)
{
	bool first = rid(n) == NT_FUNCTION_ARGS;
	if (!first && ralt_idx(n) == 1)
		return;

	compile_args(c, rchild(n, first ? 1 : 2), current, arg_count);

	compile_expression(c, rchild(n, first ? 0 : 1), current, false);

	(*arg_count)++;
}

static void compile_var_decl(struct chunk *c,
			     struct rdesc_node n,
			     struct compiler *current)
{
	update_line(rchild(n, 0));

	struct rdesc_node optasgn = rchild(n, 2);
	if (ralt_idx(optasgn) == 0) {
		compile_expression(c, rchild(optasgn, 1), current, false);
	} else {
		emit_inst(OP_NIL);
	}

	uint32_t ident_id = SEMINFO_IDENT_ID(rchild(n, 1));
	compiler_emit_define_variable_inst(current, c, ident_id);
}

static void compile_block(struct chunk *c,
			  struct rdesc_node n,
			  struct compiler *current)
{
	n = rchild(n, 1);

	compiler_begin_scope(current);

	while (ralt_idx(n) == 0){
		compile_decl(c, rchild(n, 0), current);

		n = rchild(n, 1);
	}

	compiler_end_scope(current, c);
}

static void compile_if_stmt(struct chunk *c,
			    struct rdesc_node n,
			    struct compiler *current)
{
	compile_expression(c, rchild(n, 2), current, false);

	emit_inst(OP_JUMP_IF_FALSE);
	size_t if_inst = chunk_len(c) - inst_arg_len(OP_JUMP_IF_FALSE) - 1;

	size_t then_start = chunk_len(c);

	emit_inst(OP_POP);
	compile_stmt(c, rchild(n, 4), current);

	emit_inst(OP_JUMP);
	size_t else_inst = chunk_len(c) - inst_arg_len(OP_JUMP) - 1;

	size_t else_start = chunk_len(c);

	size_t then_end = chunk_len(c);
	emit_inst(OP_POP);

	struct rdesc_node else_n = rchild(n, 5);
	if (ralt_idx(else_n) == 0) {
		update_line(rchild(else_n, 0));
		compile_stmt(c, rchild(else_n, 1), current);
	}

	size_t else_end = chunk_len(c);

	overwrite_inst(if_inst, OP_JUMP_IF_FALSE, arg_u24(then_end - then_start));
	overwrite_inst(else_inst, OP_JUMP, arg_u24(else_end - else_start));
}

static void compile_for_stmt(struct chunk *c,
			     struct rdesc_node n,
			     struct compiler *current)
{
	compiler_begin_scope(current);

	struct rdesc_node for_stmt_decl = rchild(n, 2);
	switch (ralt_idx(for_stmt_decl)) {
	case 0:
		compile_var_decl(c, rchild(for_stmt_decl, 0), current);
		break;

	case 1:
		compile_expression(c, rchild(rchild(for_stmt_decl, 0), 0), current, false);
		emit_inst(OP_POP);  /* discard the expression result */
		break;

	default:
		break;
	}

	size_t condition_expr_start = chunk_len(c);

	struct rdesc_node condition_optexpr = rchild(n, 3);
	if (ralt_idx(condition_optexpr) == 0)
		compile_expression(c,
				   rchild(condition_optexpr, 0),
				   current,
				   false);
	else
		emit_inst(OP_TRUE);

	emit_inst(OP_JUMP_IF_FALSE);  /* to end of the for loop */
	size_t break_inst = chunk_len(c) - inst_arg_len(OP_JUMP_IF_FALSE) - 1;
	size_t for_body_start = chunk_len(c);
	emit_inst(OP_POP);

	compile_stmt(c, rchild(n, 7), current);

	struct rdesc_node increment_optexpr = rchild(n, 5);
	if (ralt_idx(increment_optexpr) == 0) {
		compile_expression(c,
				   rchild(increment_optexpr, 0),
				   current,
				   false);
		emit_inst(OP_POP);
	}

	size_t before_continue = chunk_len(c);
	emit_inst(OP_JUMP_BACK);  /* to the condition expr start */
	size_t continue_inst = chunk_len(c) - inst_arg_len(OP_JUMP_BACK) - 1;

	size_t for_body_end = chunk_len(c);
	emit_inst(OP_POP);

	overwrite_inst(break_inst, OP_JUMP_IF_FALSE, arg_u24(for_body_end - for_body_start));
	overwrite_inst(continue_inst, OP_JUMP_BACK, arg_u24(before_continue - condition_expr_start));

	compiler_end_scope(current, c);
}

static void compile_while_stmt(struct chunk *c,
			       struct rdesc_node n,
			       struct compiler *current)
{
	size_t condition_expr_start = chunk_len(c);

	compile_expression(c, rchild(n, 2), current, false);  /* condition expr */

	emit_inst(OP_JUMP_IF_FALSE);  /* to end of the for loop */
	size_t break_inst = chunk_len(c) - inst_arg_len(OP_JUMP_IF_FALSE) - 1;
	size_t while_body_start = chunk_len(c);
	emit_inst(OP_POP);

	compile_stmt(c, rchild(n, 4), current);

	size_t before_continue = chunk_len(c);
	emit_inst(OP_JUMP_BACK);  /* to the condition expr start */
	size_t continue_inst = chunk_len(c) - inst_arg_len(OP_JUMP_BACK) - 1;

	size_t while_body_end = chunk_len(c);
	emit_inst(OP_POP);

	overwrite_inst(break_inst, OP_JUMP_IF_FALSE, arg_u24(while_body_end - while_body_start));
	overwrite_inst(continue_inst, OP_JUMP_BACK, arg_u24(before_continue - condition_expr_start));
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
		emit_inst(OP_POP);  /* discard the expression result */
		break;

	case NT_FOR_STMT:
		/* for ( <for_stmt_decl> <optexpression> ; <optexpression> )
		 * <stmt> */
		update_line(rchild(n, 0));
		compile_for_stmt(c, n, current);
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
		emit_inst(OP_PRINT);
		update_line(rchild(n, 2));
		break;

	case NT_RETURN_STMT:
		/* return <optexpr> ; */
		update_line(rchild(n, 0));
		compile_return_stmt(c, n, current);
		break;

	case NT_WHILE_STMT:
		/* while ( <expr> ) <stmt> */
		update_line(rchild(n, 0));
		compile_while_stmt(c, n, current);
		break;

	case NT_BLOCK:
		compile_block(c, n, current);
		break;
	}
}

static void compile_function_decl(struct chunk *c,
				  struct rdesc_node n,
				  struct compiler *current)
{
	update_line(rchild(n, 0));
	n = rchild(n, 1);

	size_t arity = 0;

	struct compiler enclosed;
	compiler_xinit(&enclosed, current);

	struct chunk new_chunk;
	chunk_xinit(&new_chunk);

	struct chunk *hold_c = c;
	struct compiler *hold_compiler = current;

	c = &new_chunk;
	current = &enclosed;

	struct rdesc_node optparams = rchild(n, 2);
	if (ralt_idx(optparams) == 0) {
		rdesc_flip_left(optparams, 0);

		struct rdesc_node params = rchild(optparams, 0);

		while (true) {
			bool last = ralt_idx(params) == 1;

			uint32_t ident_id =
				SEMINFO_IDENT_ID(rchild(params, last ? 0 : 2));

			compiler_define_local(&enclosed, ident_id);
			arity++;

			if (last)
				break;

			params = rchild(params, 0);
		}
	}

	compile_block(&new_chunk, rchild(n, 4), &enclosed);

	emit_inst(OP_NIL);
	emit_inst(OP_RETURN);

	c = hold_c;
	current = hold_compiler;

	chunk_disassemble(&new_chunk, stdout, 0, 0);
	printf("\n\n");

	uint32_t ident_id = SEMINFO_IDENT_ID(rchild(n, 1));

	struct obj_function *fun = obj_function_new(new_chunk,
						    ident_id,
						    arity,
						    fstack_len(&enclosed.upvalues));

	struct val v = OBJ_VAL((struct obj *) fun);
	uint32_t constant_id = chunk_xpush_constant(c, &v);

	compiler_emit_closure_inst(current, &enclosed, c, constant_id);
	compiler_emit_define_variable_inst(current, c, ident_id);

	compiler_destroy(&enclosed);
}

static void compile_return_stmt(struct chunk *c,
				struct rdesc_node n,
				struct compiler *current)
{
	struct rdesc_node optexpr = rchild(n, 1);
	if (ralt_idx(optexpr) == 0) {
		compile_expression(c, rchild(optexpr, 0), current, false);
	} else {
		emit_inst(OP_NIL);
	}

	emit_inst(OP_RETURN);
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
		compile_function_decl(c, rchild(n, 0), current);
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
struct chunk chunk_xcompile(struct rdesc_node n)
{
	struct chunk c;
	struct compiler current;

	chunk_xinit(&c);
	compiler_xinit(&current, NULL);

	compile_decl(&c, n, &current);
	chunk_xwrite_inst(&c, current.line, (struct inst) { .op = OP_NIL });
	chunk_xwrite_inst(&c, current.line, (struct inst) { .op = OP_RETURN });

	compiler_destroy(&current);

	return c;
}
