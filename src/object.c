#include "../include/clox.h"
#include "../include/common.h"
#include "../include/object.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


void obj_print(struct clox_value v)
{
	switch (OBJ_TYPE(v)) {
	case OBJ_STRING:
		printf("%s\n", AS_CSTRING(v));
		break;
	}
}

bool obj_is_equal(struct clox_value a, struct clox_value b)
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

void obj_free(struct clox_value v)
{
	switch (OBJ_TYPE(v)) {
	case OBJ_STRING: {
		free(AS_CSTRING(v));
		break;
	}
	}

	free(AS_OBJ(v));
}

struct obj_string *obj_string_new(char *chars)
{
	struct obj_string *obj = malloc(sizeof(struct obj_string));
	clox_assert(obj, "cannot malloc");

	*obj = (struct obj_string) {
		.obj = { .type = OBJ_STRING },
		.len = strlen(chars),
		.chars = chars
	};

	return obj;
}
