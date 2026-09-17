#include "../include/common.h"
#include "../include/object.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void obj_free(struct obj *o)
{
	switch (OBJ_TYPE(o)) {
	case OBJ_STRING: {
		free(AS_CSTRING(o));
		break;
	}
	}

	free(o);
}

struct obj *obj_clone(const struct obj *o)
{
	switch (OBJ_TYPE(o)) {
	case OBJ_STRING: {
		size_t len = AS_STRING(o)->len;
		char *chars = malloc(len + 1);

		strncpy(chars, AS_CSTRING(o), len + 1);

		return (struct obj *) obj_string_new(chars, len);
	}
	}

	return NULL;  // unreachable
}

void obj_print(const struct obj *o)
{
	switch (OBJ_TYPE(o)) {
	case OBJ_STRING:
		printf("%s\n", AS_CSTRING(o));
		break;
	}
}

bool obj_is_equal(const struct obj *a, const struct obj *b)
{
	if (OBJ_TYPE(a) != OBJ_TYPE(b))
		return false;

	switch (OBJ_TYPE(a)) {
	case OBJ_STRING: {
		struct obj_string *a_string = AS_STRING(a);
		struct obj_string *b_string = AS_STRING(b);

		return a_string->len == b_string->len &&
			memcmp(a_string->chars, b_string->chars,
			       a_string->len) == 0;
	}
	}

	return false;  // unreachable
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
