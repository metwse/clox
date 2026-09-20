#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#define OBJ_TYPE(o) (o->type)
#define IS_OBJ_TYPE(o, t) (OBJ_TYPE(o) == t)

#define AS_FUNCTION(o) ((struct obj_function *) o)
#define AS_CLOSURE(o) ((struct obj_closure *) o)
#define AS_STRING(o) ((struct obj_string *) o)
#define AS_CSTRING(o) (((struct obj_string *) o)->chars)


enum obj_type {
	OBJ_FUNCTION,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
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
	uint32_t upvalue_count;
};

struct obj_upvalue {
	struct obj obj;
	union obj_upvalue_location {
		size_t local;  /* not captured, on stack */
		struct val captured;  /* captured */
	} location;
	bool is_local;
	struct obj_upvalue *next;
};

struct obj_closure {
	struct obj obj;
	const struct obj_function *function;
	struct obj_upvalue **upvalues;
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
				      uint32_t arity,
				      uint32_t upvalue_count);

struct obj_closure *obj_closure_new(const struct obj_function *);

struct obj_upvalue *obj_upcalue_new(size_t location);

struct obj_string *obj_string_new(char *, size_t len);


#endif
