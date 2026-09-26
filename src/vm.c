#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/instructions.h"
#include "../include/globals.h"
#include "../include/object.h"
#include "../include/value.h"
#include "../include/vm.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void vm_xinit(struct vm *vm,
	      struct str_pool *strings,
	      struct globals *globals)
{
	vm->strings = strings;
	vm->globals = globals;

	fstack_call_frames_xinit(&vm->frames);
	fstack_objects_xinit(&vm->objects);
	fstack_vals_xinit(&vm->stack);
	vm->open_upvalues = NULL;
}

void vm_destroy(struct vm *vm)
{
	fstack_call_frames_destroy(&vm->frames);

	for (size_t i = 0; i < fstack_objects_len(&vm->objects); i++)
		obj_free(*fstack_objects_at_mut(&vm->objects, i));
	fstack_objects_destroy(&vm->objects);

	fstack_vals_destroy(&vm->stack);
}

struct obj *vm_obj_alloc(struct vm *vm, size_t size)
{
	struct obj *o = malloc(size);
	Lw_assert(o != NULL, "memory allocation error");

	fstack_objects_xpush(&vm->objects, &o);

	return o;
}

static void xpush(struct vm *vm, struct val *v) {
	fstack_vals_xpush(&vm->stack, v);
}

static void pop(struct vm *vm) {
	fstack_vals_pop(&vm->stack);
}

static void multipop(struct vm *vm, size_t count) {
	fstack_vals_multipop(&vm->stack, count);
}

static struct val peek(struct vm *vm, size_t distance) {
	return *fstack_vals_peek(&vm->stack, distance);
}

static struct val *peek_ref(struct vm *vm, size_t distance) {
	return fstack_vals_peek_mut(&vm->stack, distance);
}

static bool values_equal(struct val a, struct val b)
{
	if (a.type != b.type)
		return false;

	switch (a.type) {
	case VAL_NUM:
		return AS_NUM(a) == AS_NUM(b);
	case VAL_BOOL:
		return AS_BOOL(a) == AS_BOOL(b);
	case VAL_NIL:
		return true;
	case VAL_OBJ:
		return obj_is_equal(AS_OBJ(a), AS_OBJ(b));
	}

	return false; // unreachable;
}

static bool is_falsey(struct val v)
{
	return IS_NIL(v) || (IS_BOOL(v) && !AS_BOOL(v)) || (IS_NUM(v) && !AS_NUM(v));
}

static bool is_callable(struct val v)
{
	return IS_OBJ(v) && (
		IS_OBJ_TYPE(AS_OBJ(v), OBJ_CLOSURE) ||
		IS_OBJ_TYPE(AS_OBJ(v), OBJ_NATIVE_FUNCTION)
	);
}

/* TODO: query line info */
static void recover_runtime_error(struct vm *vm);
#define runtime_error(...) do { \
		Lw_report(__VA_ARGS__); \
		recover_runtime_error(vm); \
		return 1; \
	} while (0)

#define binary_op(val_type, op) do { \
		if (!IS_NUM(peek(vm, 0)) || !IS_NUM(peek(vm, 1))) { \
			runtime_error("operands must be numbers"); \
		} \
		double b = AS_NUM(peek(vm, 0)); \
		double a = AS_NUM(peek(vm, 1)); \
		multipop(vm, 2); \
		xpush(vm, &val_type(a op b)); \
	} while (0)

#define read_and_increment_pc do { \
		inst = chunk_read_inst(vm->current.c, vm->current.pc); \
		vm->current.pc += inst_len(inst); \
	} while (0)

static int call(struct vm *vm, struct obj *callable, uint32_t arg_count)
{
	switch (OBJ_TYPE(callable)) {
	case OBJ_CLOSURE: {
		struct obj_closure *closure = AS_CLOSURE(callable);
		const struct obj_function *function = closure->function;

		if (arg_count != function->arity) {
			runtime_error("expected %"PRIu32 " arguments, got %"PRIu32,
					function->arity, arg_count);
		}

		fstack_call_frames_xpush(&vm->frames, &vm->current);

		vm->current = (struct call_frame) {
			.closure = closure,
			.c = &function->chunk,
			.fp = fstack_vals_len(&vm->stack) - arg_count,
			.pc = 0,
			.arity = arg_count,
		};

		break;
	}

	case OBJ_NATIVE_FUNCTION: {
		struct obj_native_function *native_function =
			AS_NATIVE_FUNCTION(callable);

		struct val res;
		if (arg_count > 0) {
			const struct val *popped_args = peek_ref(vm, arg_count);

			struct val args[arg_count];
			for (uint32_t i = 0; i < arg_count; i++)
				args[i] = popped_args[arg_count - i];

			res = native_function->function(vm, args, arg_count);
		} else {
			res = native_function->function(vm, NULL, 0);
		}

		multipop(vm, arg_count + 1 /* pop the callable */);
		xpush(vm, &res);

		break;
	}

	default: break;  // GCOVR_EXCL: unreachable
	}

	return 0;
}

static void recover_runtime_error(struct vm *vm)
{
	fstack_vals_clear(&vm->stack);
	fstack_call_frames_clear(&vm->frames);
}

static struct obj_upvalue *capture_upvalue(struct vm *vm, size_t local)
{
	struct obj_upvalue *prev = NULL;
	struct obj_upvalue *upval = vm->open_upvalues;

	while (upval != NULL && upval->location.local > local) {
		prev = upval;
		upval = upval->next;
	}

	if (upval != NULL && upval->location.local == local)
		return upval;

	struct obj_upvalue *new_upval = obj_upvalue_new(vm, local);
	new_upval->next = upval;

	if (prev == NULL)
		vm->open_upvalues = new_upval;
	else
		prev->next = new_upval;

	return new_upval;
}

static void close_upvalues(struct vm *vm, size_t last)
{
	while (vm->open_upvalues != NULL &&
	       vm->open_upvalues->location.local >= last) {
		struct obj_upvalue *upval = vm->open_upvalues;
		upval->is_local = false;
		upval->location.captured =
			*fstack_vals_at(&vm->stack, upval->location.local);
		vm->open_upvalues = upval->next;
	}
}

static int vm_run(struct vm *vm)
{
	while (true) {
		struct inst inst;

		read_and_increment_pc;

		struct chunk *c = (struct chunk *) vm->current.c;

		switch (inst.op) {
		case OP_RETURN: {
			close_upvalues(vm, vm->current.fp);

			if (fstack_call_frames_len(&vm->frames) == 0) {
				fstack_vals_clear(&vm->stack);

				return 0;
			} else {
				/* <call-obj> <scope-leftover...> <args...> <result>*/
				struct call_frame frame =
					*fstack_call_frames_top(&vm->frames);

				*fstack_vals_at_mut(&vm->stack,
						    vm->current.fp - 1) = peek(vm, 0);
				/* overwrite function address with the result*/
				multipop(vm, fstack_vals_len(&vm->stack) - vm->current.fp);

				Lw_assert(fstack_vals_len(&vm->stack) == vm->current.fp,
					  "inconsistent stack");
				vm->current = frame;
				fstack_call_frames_pop(&vm->frames);


			}
			break;
		}

		case OP_POP:
			pop(vm);
			break;

		case OP_CLOSE_UPVALUE: {
			close_upvalues(vm, fstack_vals_len(&vm->stack) - 1);
			pop(vm);
			break;
		}

		case OP_CONSTANT:
		case OP_CONSTANT_LONG: {
			size_t constant_id = inst_get_u8_or_u24_arg(inst, 0);

			struct val v =
				*fstack_vals_at(&c->constants, constant_id);

			if (IS_OBJ(v)) {
				struct obj *new_obj = obj_clone(vm, AS_OBJ(v));
				/* vm_obj_track(vm, new_obj); */

				xpush(vm, &OBJ_VAL(new_obj));
			} else {
				xpush(vm, &v);
			}

			break;
		}

		case OP_DEFINE_GLOBAL:
		case OP_DEFINE_GLOBAL_LONG:
		case OP_GET_GLOBAL:
		case OP_GET_GLOBAL_LONG:
		case OP_SET_GLOBAL:
		case OP_SET_GLOBAL_LONG: {
			uint32_t global_id = inst_get_u8_or_u24_arg(inst, 0);
			struct val *current =
				globals_get(vm->globals, global_id);

			switch (inst.op) {
			case OP_DEFINE_GLOBAL:
			case OP_DEFINE_GLOBAL_LONG:
				if (current != NULL)
					runtime_error("variable is already defined");

				globals_define(vm->globals, global_id, peek(vm, 0));
				pop(vm);
				break;

			case OP_GET_GLOBAL:
			case OP_GET_GLOBAL_LONG:
				if (current == NULL)
					runtime_error("undefined variable");

				xpush(vm, current);
				break;

			case OP_SET_GLOBAL:
			case OP_SET_GLOBAL_LONG:
				if (current == NULL)
					runtime_error("undefined variable");

				*current = peek(vm, 0);
				break;

			default:
				break;  // unreachable
			}
			break;
		}

		case OP_GET_LOCAL:
		case OP_GET_LOCAL_LONG:
		case OP_SET_LOCAL:
		case OP_SET_LOCAL_LONG: {
			size_t slot = inst_get_u8_or_u24_arg(inst, 0);
			struct val *v = fstack_vals_at_mut(&vm->stack,
							   vm->current.fp + slot);

			if (inst.op == OP_GET_LOCAL || inst.op == OP_GET_LOCAL_LONG)
				xpush(vm, v);
			else
				*v = peek(vm, 0);
			break;
		}

		case OP_JUMP:
		case OP_JUMP_IF_FALSE:
		case OP_JUMP_BACK: {
			uint32_t jump = inst_get_u24_arg(inst, 0);

			if (inst.op == OP_JUMP_BACK)
				vm->current.pc -= jump + inst_len(inst);
			else if (inst.op == OP_JUMP ||
			    is_falsey(peek(vm, 0)))
				vm->current.pc += jump;

			break;
		}

		case OP_CALL: {
			uint32_t arg_count = inst_get_u8_arg(inst, 0);

			struct val v = peek(vm, arg_count);
			if (!is_callable(v))
				runtime_error("expression result is not callable");

			if (call(vm, AS_OBJ(v), arg_count))
				return 1;

			break;
		}

		case OP_GET_UPVALUE:
		case OP_GET_UPVALUE_LONG:
		case OP_SET_UPVALUE:
		case OP_SET_UPVALUE_LONG: {
			size_t slot = inst_get_u8_or_u24_arg(inst, 0);

			struct obj_upvalue *upval =
				vm->current.closure->upvalues[slot];
			struct val *v;

			if (upval->is_local)
				v = fstack_vals_at_mut(&vm->stack,
						       upval->location.local);
			else
				v = &upval->location.captured;

			if (inst.op == OP_GET_UPVALUE || inst.op == OP_GET_UPVALUE_LONG)
				xpush(vm, v);
			else
				*v = peek(vm, 0);

			break;
		}

		case OP_CLOSURE:
		case OP_CLOSURE_LONG: {
			size_t constant_id = inst_get_u8_or_u24_arg(inst, 0);

			struct val v = *fstack_vals_at(&c->constants,
						       constant_id);

			Lw_assert(IS_OBJ(v) && IS_OBJ_TYPE(AS_OBJ(v), OBJ_FUNCTION),
				  "can only create closures from functions");

			struct obj_closure *closure =
				obj_closure_new(vm, AS_FUNCTION(AS_OBJ(v)));

			size_t offset = inst_arg_len(inst.op), i = 0;
			uint32_t index, arg_len;
			bool is_local;

			while ((arg_len = inst_get_next_closure_arg(inst,
								    offset,
								    &index,
								    &is_local) &&
				i < closure->function->upvalue_count)) {

				if (is_local) {
					closure->upvalues[i] =
						capture_upvalue(vm,
								vm->current.fp + index);
				} else {
					closure->upvalues[i] =
						vm->current.closure->upvalues[index];
				}

				i++;
				offset += arg_len + 1;
			}

			xpush(vm, &OBJ_VAL((struct obj *) closure));

			break;
		}

		case OP_NIL: xpush(vm, &NIL_VAL); break;
		case OP_TRUE: xpush(vm, &BOOL_VAL(true)); break;
		case OP_FALSE: xpush(vm, &BOOL_VAL(false)); break;

		case OP_EQUAL: {
			struct val b = peek(vm, 0);
			struct val a = peek(vm, 1);

			bool res = values_equal(a, b);
			multipop(vm, 2);
			xpush(vm, &BOOL_VAL(res));
			break;
		}

		case OP_GREATER: binary_op(BOOL_VAL, >); break;
		case OP_LESS: binary_op(BOOL_VAL, <); break;

		case OP_ADD: binary_op(NUM_VAL, +); break;
		case OP_SUBSTRACT: binary_op(NUM_VAL, -); break;
		case OP_MULTIPLY: binary_op(NUM_VAL, *); break;
		case OP_DIVIDE: binary_op(NUM_VAL, /); break;

		case OP_AND:
		case OP_OR: {
			struct val b = peek(vm, 0);
			struct val a = peek(vm, 1);
			bool res;
			if (inst.op == OP_AND)
				res = !is_falsey(a) && !is_falsey(b);
			else
				res = !is_falsey(a) || !is_falsey(b);
			multipop(vm, 2);
			xpush(vm, &BOOL_VAL(res));
			break;
		}

		case OP_NOT: {
			bool res = is_falsey(peek(vm, 0));
			pop(vm);
			xpush(vm, &BOOL_VAL(res));
			break;
		}

		case OP_NEGATE: {
			if (!IS_NUM(peek(vm, 0)))
				runtime_error("can only negatate numers");

			double res = -AS_NUM(peek(vm, 0));
			pop(vm);
			xpush(vm, &NUM_VAL(res));
			break;
		}
		}
	}
}

int vm_execute(struct vm *vm, const struct chunk *c)
{
	vm->current = (struct call_frame) {
		.closure = NULL,
		.c = c,
		.arity = 0,
		.fp = 0,
		.pc = 0
	};

	return vm_run(vm);
}
