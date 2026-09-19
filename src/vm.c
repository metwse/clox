#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/instructions.h"
#include "../include/value.h"
#include "../include/vm.h"

#include "../vendor/libfun/include/stack.h"
#include "../vendor/libfun/include/hashmap.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


void vm_xinit(struct vm *vm)
{
	fstack_xinit(&vm->stack, sizeof(struct val));
	fstack_xinit(&vm->objects, sizeof(struct obj *));
	fhashmap_xinit(&vm->globals, sizeof(struct val));
}

void vm_destroy(struct vm *vm)
{
	fstack_destroy(&vm->stack);

	for (size_t i = 0; i < fstack_len(&vm->objects); i++)
		obj_free(*(struct obj **) fstack_at(&vm->objects, i));

	fstack_destroy(&vm->objects);
	fhashmap_destroy(&vm->globals);
}

void vm_set_chunk(struct vm *vm, const struct chunk *c)
{
	vm->current_chunk = c;
	vm->pc = 0;
}

void vm_obj_track(struct vm *vm, struct obj *o)
{
	fstack_xpush(&vm->objects, &o);
}

static void xpush(struct vm *vm, struct val *v) {
	fstack_xpush(&vm->stack, v);
}

static struct val pop(struct vm *vm) {
	return *(struct val *) fstack_pop(&vm->stack);
}

static struct val peek(struct vm *vm, size_t distance) {
	return *(struct val *) fstack_at(&vm->stack,
					 fstack_len(&vm->stack) - distance - 1);
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

// TODO: query line info
#define runtime_error(...) do { \
		clox_report(__VA_ARGS__); \
		recover_runtime_error(vm); \
		return 1; \
	} while (0);

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
		inst = chunk_read_inst(vm->current_chunk, vm->pc); \
		vm->pc += 1 + inst_arg_len(inst.op); \
	} while (0)

static void recover_runtime_error(struct vm *vm)
{
	fstack_destroy(&vm->stack);
	fstack_xinit(&vm->stack, sizeof(struct val));
}

int vm_run(struct vm *vm)
{
	while (true) {
		struct inst inst;

		read_and_increment_pc;

		struct chunk *c = (struct chunk *) vm->current_chunk;

		switch (inst.op) {
		case OP_RETURN:
			return 0;

		case OP_PRINT:
			print_val(pop(vm));
			break;

		case OP_POP:
			pop(vm);
			break;

		case OP_CONSTANT:
		case OP_CONSTANT_LONG: {
			size_t constant_idx = get_u8_or_u24_arg(inst, 0);

			struct val v = *(struct val *) fstack_at(&c->constants,
								 constant_idx);

			if (IS_OBJ(v)) {
				struct obj *new_obj = obj_clone(AS_OBJ(v));
				vm_obj_track(vm, new_obj);

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
			uint32_t constant_id = get_u8_or_u24_arg(inst, 0);

			struct val *current = fhashmap_get2(&vm->globals,
							    &constant_id,
							    sizeof(uint32_t));

			switch (inst.op) {
			case OP_DEFINE_GLOBAL:
			case OP_DEFINE_GLOBAL_LONG:
				if (current != NULL)
					runtime_error("variable is already defined");

				struct val v = pop(vm);
				fhashmap_xinsert2(&vm->globals,
						  &constant_id,
						  (sizeof(uint32_t)),
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
			struct val *v = fstack_at(&vm->stack, slot);


			if (inst.op == OP_GET_LOCAL || inst.op == OP_GET_LOCAL_LONG)
				xpush(vm, v);
			else
				*v = peek(vm, 0);
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
