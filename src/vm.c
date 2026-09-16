#include "../include/clox.h"
#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/instructions.h"
#include "../include/vm.h"

#include "../vendor/libfun/include/stack.h"

#include <stdbool.h>
#include <stdint.h>


void vm_xinit(struct vm *vm)
{
	fstack_xinit(&vm->stack, sizeof(struct clox_value));
}

void vm_destroy(struct vm *vm)
{
	fstack_destroy(&vm->stack);
}

void vm_set_chunk(struct vm *vm, const struct chunk *c)
{
	vm->current_chunk = c;
	vm->pc = 0;
}

static void xpush(struct vm *vm, struct clox_value *v) {
	fstack_xpush(&vm->stack, v);
}

static struct clox_value pop(struct vm *vm) {
	return *(struct clox_value *) fstack_pop(&vm->stack);
}

static struct clox_value peek(struct vm *vm, size_t distance) {
	return *(struct clox_value *) fstack_at(&vm->stack,
						fstack_len(&vm->stack) - distance);
}

static bool values_equal(struct clox_value a, struct clox_value b)
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
	default:
		return false; // unreachable;
	}

}

// TODO: query line info
#define runtime_error(...) do { \
		clox_report(__VA_ARGS__); \
		return 1; \
	} while (0);

#define binary_op(val_type, op) do { \
		if (!IS_NUM(peek(vm, 0)) || !IS_NUM(peek(vm, 1))) { \
			runtime_error("Operands must be numbers."); \
		} \
		double b = AS_NUM(pop(vm)); \
		double a = AS_NUM(pop(vm)); \
		xpush(vm, &val_type(a op b)); \
	} while (0)

int vm_run(struct vm *vm)
{
	while (true) {
		struct inst inst = chunk_read_inst(vm->current_chunk, vm->pc);
		struct chunk *c = (struct chunk *) vm->current_chunk;

		uint32_t constant_index;
		bool constant_index_init = false;

		switch (inst.op) {
		case OP_RETURN:
			return 0;

		case OP_CONSTANT:
			constant_index = inst_get_char_arg(inst, 0);
			constant_index_init = true;

		// fallthrough
		case OP_CONSTANT_LONG:
			if (!constant_index_init)
				constant_index = inst_get_u24_arg(inst, 0);

			xpush(vm, fstack_at(&c->constants, constant_index));
			break;

		case OP_NIL: xpush(vm, &NIL_VAL); break;
		case OP_TRUE: xpush(vm, &BOOL_VAL(true)); break;
		case OP_FALSE: xpush(vm, &BOOL_VAL(false)); break;

		case OP_EQUAL: {
			struct clox_value b = pop(vm);
			struct clox_value a = pop(vm);

			xpush(vm, &BOOL_VAL(values_equal(a, b)));

			break;
		}
		case OP_GREATER: binary_op(NUM_VAL, >); break;
		case OP_LESS: binary_op(NUM_VAL, <); break;

		case OP_ADD: binary_op(NUM_VAL, +); break;
		case OP_SUBSTRACT: binary_op(NUM_VAL, -); break;
		case OP_MULTIPLY: binary_op(NUM_VAL, *); break;
		case OP_DIVIDE: binary_op(NUM_VAL, /); break;

		case OP_NEGATE:
			if (!IS_NUM(peek(vm, 0)))
				runtime_error("Only negatate numers.");

			xpush(vm, &NUM_VAL(-AS_NUM(pop(vm))));

			break;
		}
	}
}
