#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/object.h"
#include "../include/string_pool.h"
#include "../include/vm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void obj_free(struct obj *o)
{
	switch (OBJ_TYPE(o)) {
	case OBJ_FUNCTION:
		chunk_destroy(&AS_FUNCTION(o)->chunk);
		break;

	case OBJ_UPVALUE:
		break;

	case OBJ_CLOSURE:
		free(AS_CLOSURE(o)->upvalues);
		break;

	case OBJ_NATIVE_FUNCTION:
		break;

	case OBJ_STR_LITERAL:
		break;
	}

	free(o);
}

struct obj *obj_clone(struct vm *vm, const struct obj *o)
{
	switch (OBJ_TYPE(o)) {
	case OBJ_FUNCTION:
		Lw_fatal("functions are not clonable");
		break;

	case OBJ_CLOSURE:
		Lw_fatal("closures are not clonable");
		break;

	case OBJ_UPVALUE:
		Lw_fatal("closures are not clonable");
		break;

	case OBJ_NATIVE_FUNCTION:
		Lw_fatal("native functions are not clonable");
		break;

	case OBJ_STR_LITERAL:
		return (struct obj *) obj_str_literal_new(vm, AS_STR_LITERAL(o)->id);
	}

	return NULL;  // unreachable
}

void obj_print(const struct obj *o, const struct vm *vm)
{
	switch (OBJ_TYPE(o)) {
	case OBJ_FUNCTION:
		printf("<fn>\n");
		break;

	case OBJ_CLOSURE:
		obj_print((const struct obj *) AS_CLOSURE(o)->function, vm);
		break;

	case OBJ_NATIVE_FUNCTION:
		printf("<native fn>\n");
		break;

	case OBJ_UPVALUE:
		break;

	case OBJ_STR_LITERAL: {
		const char *out_chars;
		size_t out_len;

		Lw_assert(str_pool_get_chars(vm->strings,
					     AS_STR_LITERAL(o)->id,
					     &out_chars,
					     &out_len),
			    "str literal not found, possibly GC'ed!");
		printf("%.*s\n", (int) out_len, out_chars);
		break;
	}
	}
}

bool obj_is_equal(const struct obj *a, const struct obj *b)
{
	/* TODO: check str_literal - string */
	if (OBJ_TYPE(a) != OBJ_TYPE(b))
		return false;

	switch (OBJ_TYPE(a)) {
	case OBJ_FUNCTION:
	case OBJ_UPVALUE:
	case OBJ_CLOSURE:
	case OBJ_NATIVE_FUNCTION:
		return a == b;

	case OBJ_STR_LITERAL:
		return AS_STR_LITERAL(a)->id == AS_STR_LITERAL(b)->id;
	}

	return false;  // unreachable
}

#define obj_new(type) \
	struct obj_ ## type *obj = \
		(struct obj_ ##type *) vm_obj_alloc(vm, \
						    sizeof(struct obj_ ## type)); \
	obj->obj.is_marked = false

struct obj_function *obj_function_new(struct vm *vm,
				      struct chunk c,
				      uint32_t arity,
				      uint32_t upvalue_count)
{
	obj_new(function);

	*obj = (struct obj_function) {
		.obj = { .type = OBJ_FUNCTION },
		.chunk = c,
		.arity = arity,
		.upvalue_count = upvalue_count
	};

	return obj;
}

struct obj_closure *obj_closure_new(struct vm *vm,
				    const struct obj_function *function)
{
	obj_new(closure);

	struct obj_upvalue **upvalues;
	if (function->upvalue_count)
		upvalues = malloc(sizeof(struct obj_upvalue *) *
					function->upvalue_count);
	else
		upvalues = NULL;

	Lw_assert(obj, "cannot malloc");

	*obj = (struct obj_closure) {
		.obj = { .type = OBJ_CLOSURE },
		.function = function,
		.upvalues = upvalues
	};

	return obj;
}

struct obj_upvalue *obj_upvalue_new(struct vm *vm, size_t location)
{
	obj_new(upvalue);

	*obj = (struct obj_upvalue) {
		.obj = { .type = OBJ_UPVALUE },
		.location.local = location,
		.is_local = true,
		.next = NULL
	};

	return obj;
}

struct obj_native_function *obj_native_function_new(struct vm *vm,
						    native_function_t *native_function)
{
	obj_new(native_function);

	*obj = (struct obj_native_function) {
		.obj = { .type = OBJ_NATIVE_FUNCTION },
		.function = native_function,
	};

	return obj;
}

struct obj_str_literal *obj_str_literal_new(struct vm *vm, uint32_t id)
{
	obj_new(str_literal);

	*obj = (struct obj_str_literal) {
		.obj = { .type = OBJ_STR_LITERAL },
		.id = id,
	};

	return obj;
}
