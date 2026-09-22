#include "compiler_internal.h"

#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/instructions.h"

#include "../vendor/libfun/include/stack.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>


static uint32_t resolve_local(struct compiler *, uint32_t);
static uint32_t resolve_upvalue(struct compiler *, uint32_t);
static uint32_t add_upvalue(struct compiler *, uint32_t, bool);


void compiler_xinit(struct compiler *current, struct compiler *enclosing)
{
	current->enclosing = enclosing;
	current->line = 0;
	current->scope_depth = 0;
	fstack_locals_xinit(&current->locals);
	fstack_upvalues_xinit(&current->upvalues);
}

void compiler_destroy(struct compiler *current)
{
	fstack_locals_destroy(&current->locals);
	fstack_upvalues_destroy(&current->upvalues);
}

void compiler_emit_variable_inst(struct compiler *current,
				 struct chunk *c,
				 bool is_set,
				 uint32_t ident_id)
{
	enum opcode set_op = OP_SET_GLOBAL;
	enum opcode get_op = OP_GET_GLOBAL;

	uint32_t arg;
	if ((arg = resolve_local(current, ident_id)) != UINT_MAX) {
		set_op = OP_SET_LOCAL;
		get_op = OP_GET_LOCAL;
	} else if ((arg = resolve_upvalue(current, ident_id)) != UINT_MAX) {
		set_op = OP_SET_UPVALUE;
		get_op = OP_GET_UPVALUE;
	} else {
		arg = ident_id;
	}

	enum opcode op = is_set ? set_op : get_op;

	emit_inst_u8or24(op, arg);
}

void compiler_emit_define_variable_inst(struct compiler *current,
					struct chunk *c,
					uint32_t ident_id)
{

	uint32_t local_slot = resolve_local(current, ident_id);
	if (local_slot == UINT16_MAX || current->scope_depth == 0)
		emit_inst_u8or24(OP_DEFINE_GLOBAL, ident_id);
	else
		compiler_define_local(current, ident_id);
}

void compiler_emit_closure_inst(struct compiler *current,
				struct compiler *enclosed,
				struct chunk *c,
				uint32_t constant_id)
{
	size_t len = constant_id < 256 ? 1 : 3;
	for (size_t i = 0; i < fstack_upvalues_len(&enclosed->upvalues); i++) {
		const struct upvalue *upval =
			fstack_upvalues_at(&enclosed->upvalues, i);

		len += (upval->index < 256 ? 1 : 3) + 1;
	}
	len++;

	uint8_t args[len];

	memcpy(&args, arg_u8or24(constant_id), constant_id < 256 ? 1 : 3);
	size_t offset = constant_id < 256 ? 1 : 3;
	for (size_t i = 0; i < fstack_upvalues_len(&enclosed->upvalues); i++) {
		const struct upvalue *upval =
			fstack_upvalues_at(&enclosed->upvalues, i);

		union op_closure_arg arg;
		arg.last = false;
		arg.upvalue.is_local = upval->is_local;
		arg.upvalue.is_long = upval->index > 255;

		memcpy(&args[offset], &arg, sizeof(union op_closure_arg));
		memcpy(&args[offset + sizeof(union op_closure_arg)],
		       arg_u8or24(upval->index),
		       upval->index < 256 ? 1 : 3);

		offset += sizeof(union op_closure_arg) +
			  (upval->index < 256 ? 1 : 3);
	}
	args[len - 1] = 0xFF;

	emit_inst_args(OP_CLOSURE, args);
}

void compiler_begin_scope(struct compiler *current)
{
	current->scope_depth++;
}

void compiler_end_scope(struct compiler *current, struct chunk *c)
{
	/* pop local variables */
	const struct local *local;
	while (fstack_locals_len(&current->locals) > 0 &&
	       (local = fstack_locals_top(&current->locals)) &&
	       local->depth == current->scope_depth) {
		fstack_locals_pop(&current->locals);

		if (local->is_captured)
			emit_inst(OP_CLOSE_UPVALUE);
		else
			emit_inst(OP_POP);
	}
	current->scope_depth--;
}

void compiler_define_local(struct compiler *current, uint32_t ident_id)
{
	fstack_locals_xpush(&current->locals,
			    &(struct local) {
				    .ident_id = ident_id,
				    .depth = current->scope_depth,
				    .is_captured = false
			    });
}

static uint32_t add_upvalue(struct compiler *current,
			    uint32_t index,
			    bool is_local)
{
	uint32_t upvalue_count = fstack_upvalues_len(&current->upvalues);

	for (size_t i = 0; i < upvalue_count; i++) {
		const struct upvalue *upval =
			fstack_upvalues_at(&current->upvalues, i);

		if (upval->index == index && upval->is_local == is_local)
			return i;
	}

	fstack_upvalues_xpush(&current->upvalues,
			      &(struct upvalue) {
				      .index = index,
				      .is_local = is_local,
			      });

	return upvalue_count;
}

static uint32_t resolve_upvalue(struct compiler *current, uint32_t ident_id)
{
	if (current->enclosing == NULL)
		return UINT_MAX;

	uint32_t local = resolve_local(current->enclosing, ident_id);
	if (local != UINT_MAX) {
		struct local *local_v =
			fstack_locals_at_mut(&current->enclosing->locals, local);
		local_v->is_captured = true;
		return add_upvalue(current, local, true);
	}

	uint32_t upvalue = resolve_upvalue(current->enclosing, ident_id);
	if (upvalue != UINT_MAX) {
		return add_upvalue(current, upvalue, false);
	}

	return UINT_MAX;
}

static uint32_t resolve_local(struct compiler *current, uint32_t ident_id)
{
	for (size_t i = fstack_locals_len(&current->locals); i > 0; i--) {
		const struct local *var =
			fstack_locals_at(&current->locals, i - 1);
		if (var->ident_id == ident_id)
			return i - 1;
	}

	return UINT_MAX;
}
