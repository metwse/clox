#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#define OBJ_TYPE(o) (o->type)
#define IS_OBJ_TYPE(o, t) (OBJ_TYPE(o) == t)

#define AS_FUNCTION(o) ((struct obj_function *) o)
#define AS_STRING(o) ((struct obj_string *) o)
#define AS_CSTRING(o) (((struct obj_string *) o)->chars)


enum obj_type {
	OBJ_FUNCTION,
	OBJ_STRING,
};

struct obj {
	enum obj_type type;
};


struct obj_function {
	struct obj obj;
	uint32_t arity;
	struct chunk chunk;
	uint32_t ident_id;
};

struct obj_string {
	struct obj obj;
	size_t len;
	char *chars;
};


void obj_free(struct obj *);

struct obj *obj_clone(const struct obj *);

void obj_print(const struct obj *);

bool obj_is_equal(const struct obj *a, const struct obj *b);


struct obj_function *obj_function_new(struct chunk,
				      uint32_t ident_id,
				      size_t arity);

struct obj_string *obj_string_new(char *, size_t len);


#endif
