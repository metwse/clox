#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/object.h"
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

	case OBJ_STRING: {
		free(AS_CSTRING(o));
		break;
	}

	case OBJ_STR_LITERAL:
		break;
	}

	free(o);
}

struct obj *obj_clone(const struct obj *o)
{
	switch (OBJ_TYPE(o)) {
	case OBJ_FUNCTION:
		clox_fatal("functions are not clonable");
		break;

	case OBJ_CLOSURE:
		clox_fatal("closures are not clonable");
		break;

	case OBJ_UPVALUE:
		clox_fatal("closures are not clonable");
		break;

	case OBJ_STRING: {
		size_t len = AS_STRING(o)->len;
		char *chars = malloc(len + 1);

		strncpy(chars, AS_CSTRING(o), len + 1);

		return (struct obj *) obj_string_new(chars, len);
	}

	case OBJ_STR_LITERAL:
		return (struct obj *) obj_str_literal_new(AS_STR_LITERAL(o)->id);
	}

	return NULL;  // unreachable
}

void obj_print(const struct obj *o, const struct vm *vm)
{
	switch (OBJ_TYPE(o)) {
	case OBJ_FUNCTION:
		printf("(fn %d)\n", AS_FUNCTION(o)->name_str_id);
		break;

	case OBJ_CLOSURE:
		obj_print((const struct obj *) AS_CLOSURE(o)->function, vm);
		break;

	case OBJ_UPVALUE:
		break;

	case OBJ_STRING:
		printf("%s\n", AS_CSTRING(o));
		break;

	case OBJ_STR_LITERAL: {
		const char *out_chars;
		size_t out_len;

		clox_assert(str_pool_get_chars(vm->strings,
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
		return a == b;

	case OBJ_STRING: {
		struct obj_string *a_string = AS_STRING(a);
		struct obj_string *b_string = AS_STRING(b);

		return a_string->len == b_string->len &&
			memcmp(a_string->chars, b_string->chars,
			       a_string->len) == 0;
	}

	case OBJ_STR_LITERAL:
		return AS_STR_LITERAL(a)->id == AS_STR_LITERAL(b)->id;
	}

	return false;  // unreachable
}

struct obj_function *obj_function_new(struct chunk c,
				      uint32_t arity,
				      uint32_t upvalue_count,
				      uint32_t name_str_id)
{
	struct obj_function *obj = malloc(sizeof(struct obj_function));
	clox_assert(obj, "cannot malloc");

	*obj = (struct obj_function) {
		.obj = { .type = OBJ_FUNCTION },
		.name_str_id = name_str_id,
		.chunk = c,
		.arity = arity,
		.upvalue_count = upvalue_count
	};

	return obj;
}

struct obj_closure *obj_closure_new(const struct obj_function *function)
{
	struct obj_closure *obj = malloc(sizeof(struct obj_closure));
	clox_assert(obj, "cannot malloc");

	struct obj_upvalue **upvalues;
	if (function->upvalue_count)
		upvalues = malloc(sizeof(struct obj_upvalue *) *
					function->upvalue_count);
	else
		upvalues = NULL;

	clox_assert(obj, "cannot malloc");

	*obj = (struct obj_closure) {
		.obj = { .type = OBJ_CLOSURE },
		.function = function,
		.upvalues = upvalues
	};

	return obj;
}

struct obj_upvalue *obj_upcalue_new(size_t location)
{
	struct obj_upvalue *obj = malloc(sizeof(struct obj_upvalue));
	clox_assert(obj, "cannot malloc");

	*obj = (struct obj_upvalue) {
		.obj = { .type = OBJ_UPVALUE },
		.location.local = location,
		.is_local = true,
		.next = NULL
	};

	return obj;
}

struct obj_string *obj_string_new(char *chars, size_t len)
{
	struct obj_string *obj = malloc(sizeof(struct obj_string));
	clox_assert(obj, "cannot malloc");

	*obj = (struct obj_string) {
		.obj = { .type = OBJ_STRING },
		.len = len,
		.chars = chars
	};

	return obj;
}

struct obj_str_literal *obj_str_literal_new(uint32_t id)
{
	struct obj_str_literal *obj = malloc(sizeof(struct obj_str_literal));
	clox_assert(obj, "cannot malloc");

	*obj = (struct obj_str_literal) {
		.obj = { .type = OBJ_STR_LITERAL },
		.id = id,
	};

	return obj;
}
