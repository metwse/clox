#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/instructions.h"
#include "../include/value.h"
#include "../include/vm.h"

#include "../vendor/libfun/include/stack.h"
#include "../vendor/libfun/include/hashmap.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


void vm_xinit(struct vm *vm)
{
	fstack_call_frame_xinit(&vm->frames);
	fstack_val_xinit(&vm->stack);
	/* fstack_xinit(&vm->objects, sizeof(struct obj *)); */
	fhashmap_global_xinit(&vm->globals);
	vm->open_upvalues = NULL;
}

void vm_destroy(struct vm *vm)
{
	fstack_call_frame_destroy(&vm->frames);
	fstack_val_destroy(&vm->stack);

	/*
	for (size_t i = 0; i < fstack_len(&vm->objects); i++)
		obj_free(*(struct obj **) fstack_at(&vm->objects, i));
	fstack_destroy(&vm->objects);
	*/

	fhashmap_global_destroy(&vm->globals);
}

/*
void vm_obj_track(struct vm *vm, struct obj *o)
{
	fstack_xpush(&vm->objects, &o);
}
*/

static void xpush(struct vm *vm, struct val *v) {
	fstack_val_xpush(&vm->stack, v);
}

static struct val pop(struct vm *vm) {
	return *fstack_val_pop(&vm->stack);
}

static struct val peek(struct vm *vm, size_t distance) {
	return *fstack_val_peek(&vm->stack, distance);
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

static void print_val(struct val v)
{
	switch (v.type) {
	case VAL_NUM:
		printf("%g\n", AS_NUM(v));
		break;

	case VAL_BOOL:
		printf(AS_BOOL(v) ? "true\n" : "false\n");
		break;

	case VAL_NIL:
		printf("nil\n");
		break;

	case VAL_OBJ:
		obj_print(AS_OBJ(v));
		break;
	}
}

static bool is_falsey(struct val v)
{
	return IS_NIL(v) || (IS_BOOL(v) && !AS_BOOL(v)) || (IS_NUM(v) && !AS_NUM(v));
}

static bool is_callable(struct val v)
{
	return IS_OBJ(v) && IS_OBJ_TYPE(AS_OBJ(v), OBJ_CLOSURE);
}

/* TODO: query line info */
static void recover_runtime_error(struct vm *vm);
#define runtime_error(...) do { \
		clox_report(__VA_ARGS__); \
		recover_runtime_error(vm); \
		return 1; \
	} while (0)

#define binary_op(val_type, op) do { \
		if (!IS_NUM(peek(vm, 0)) || !IS_NUM(peek(vm, 1))) { \
			runtime_error("operands must be numbers"); \
		} \
		double b = AS_NUM(pop(vm)); \
		double a = AS_NUM(pop(vm)); \
		xpush(vm, &val_type(a op b)); \
	} while (0)

#define get_u8_or_u24_arg(inst, offset) \
	(inst_arg_len(inst.op) == 3 ? \
	 	inst_get_u24_arg(inst, offset) : inst_get_u8_arg(inst, offset))

#define read_and_increment_pc do { \
		inst = chunk_read_inst(vm->current.c, vm->current.pc); \
		vm->current.pc += inst_len(inst); \
	} while (0)

static int call(struct vm *vm, struct obj_closure *closure, uint32_t arg_count)
{
	const struct obj_function *function = closure->function;

	if (arg_count != function->arity) {
		runtime_error("expected %"PRIu32 " arguments, got %"PRIu32,
			      function->arity, arg_count);
	}

	fstack_call_frame_xpush(&vm->frames, &vm->current);

	vm->current = (struct call_frame) {
		.closure = closure,
		.c = &function->chunk,
		.fp = fstack_val_len(&vm->stack) - arg_count,
		.pc = 0,
		.arity = arg_count,
	};

	return 0;
}

static void recover_runtime_error(struct vm *vm)
{
	fstack_val_destroy(&vm->stack);
	fstack_val_xinit(&vm->stack);
	/* TODO: continue from the previous function frame */
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

	struct obj_upvalue *new_upval = obj_upcalue_new(local);
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
			*fstack_val_at(&vm->stack, upval->location.local);
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
			if (fstack_call_frame_len(&vm->frames) == 0) {
				print_val(pop(vm));

				clox_assert(fstack_val_len(&vm->stack) == 0,
					    "inconsistent stack");

				return 0;
			} else {
				struct val res = pop(vm);

				for (uint32_t arity = vm->current.arity;
				     arity > 0;
				     arity--) {
					pop(vm);
				}

				pop(vm);  /* the callable */

				xpush(vm, &res);

				vm->current = *fstack_call_frame_pop(&vm->frames);
			}
			break;
		}

		case OP_PRINT:
			print_val(pop(vm));
			break;

		case OP_POP:
			pop(vm);
			break;

		case OP_CLOSE_UPVALUE: {
			close_upvalues(vm, fstack_val_len(&vm->stack) - 1);
			pop(vm);
			break;
		}

		case OP_CONSTANT:
		case OP_CONSTANT_LONG: {
			size_t constant_idx = get_u8_or_u24_arg(inst, 0);

			struct val v =
				*fstack_val_at(&c->constants, constant_idx);

			if (IS_OBJ(v)) {
				struct obj *new_obj = obj_clone(AS_OBJ(v));
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
			uint32_t ident_id = get_u8_or_u24_arg(inst, 0);

			struct val *current =
				fhashmap_global_get2_mut(&vm->globals, &ident_id);

			switch (inst.op) {
			case OP_DEFINE_GLOBAL:
			case OP_DEFINE_GLOBAL_LONG:
				if (current != NULL)
					runtime_error("variable is already defined");

				struct val v = pop(vm);
				fhashmap_global_xinsert2(&vm->globals,
							 &ident_id,
							 &v);

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
			size_t slot = get_u8_or_u24_arg(inst, 0);
			struct val *v = fstack_val_at_mut(&vm->stack,
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

			struct obj_closure *closure = AS_CLOSURE(AS_OBJ(v));

			if (call(vm, closure, arg_count))
				return 1;

			break;
		}

		case OP_GET_UPVALUE:
		case OP_GET_UPVALUE_LONG:
		case OP_SET_UPVALUE:
		case OP_SET_UPVALUE_LONG: {
			size_t slot = get_u8_or_u24_arg(inst, 0);

			struct obj_upvalue *upval =
				vm->current.closure->upvalues[slot];

			struct val *v;

			if (upval->is_local)
				v = fstack_val_at_mut(&vm->stack,
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
			size_t constant_idx = get_u8_or_u24_arg(inst, 0);

			struct val v = *fstack_val_at(&c->constants,
						      constant_idx);

			clox_assert(IS_OBJ(v) && IS_OBJ_TYPE(AS_OBJ(v), OBJ_FUNCTION),
				    "can only create closures from functions");

			struct obj_closure *closure =
				obj_closure_new(AS_FUNCTION(AS_OBJ(v)));

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
			struct val b = pop(vm);
			struct val a = pop(vm);

			xpush(vm, &BOOL_VAL(values_equal(a, b)));
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
			struct val b = pop(vm);
			struct val a = pop(vm);
			if (inst.op == OP_AND)
				xpush(vm, &BOOL_VAL(!is_falsey(a) && !is_falsey(b)));
			else
				xpush(vm, &BOOL_VAL(!is_falsey(a) || !is_falsey(b)));
			break;
		}

		case OP_NOT:
			xpush(vm, &BOOL_VAL(is_falsey(pop(vm))));
			break;

		case OP_NEGATE:
			if (!IS_NUM(peek(vm, 0)))
				runtime_error("can only negatate numers");

			xpush(vm, &NUM_VAL(-AS_NUM(pop(vm))));
			break;
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
