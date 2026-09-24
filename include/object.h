#ifndef OBJECT_H
#define OBJECT_H

#include "chunk.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

struct vm;  /* defined in vm.h */


#define OBJ_TYPE(o) (o->type)
#define IS_OBJ_TYPE(o, t) (OBJ_TYPE(o) == t)

#define AS_FUNCTION(o) ((struct obj_function *) o)
#define AS_CLOSURE(o) ((struct obj_closure *) o)
#define AS_UPVALUE(o) ((struct obj_upvalue *) o)
#define AS_STR_LITERAL(o) ((struct obj_str_literal *) o)
#define AS_NATIVE_FUNCTION(o) ((struct obj_native_function *) o)


enum obj_type {
	OBJ_FUNCTION,
	OBJ_CLOSURE,
	OBJ_UPVALUE,
	OBJ_NATIVE_FUNCTION,
	OBJ_STR_LITERAL,
};

struct obj {
	enum obj_type type;
	bool is_marked;
};


typedef struct val native_function_t(struct vm *vm,
				     struct val argv[],
				     uint32_t argc);


struct obj_function {
	struct obj obj;
	struct chunk chunk;
	uint32_t arity;
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

struct obj_native_function {
	struct obj obj;
	native_function_t *function;
};

struct obj_str_literal {
	struct obj obj;
	uint32_t id;
};


void obj_free(struct obj *);

void obj_print(const struct obj *, const struct vm *);

bool obj_is_equal(const struct obj *a, const struct obj *b);


struct obj *obj_clone(struct vm *, const struct obj *);

struct obj_function *obj_function_new(struct vm *,
				      struct chunk,
				      uint32_t arity,
				      uint32_t upvalue_count);

struct obj_closure *obj_closure_new(struct vm *,
				    const struct obj_function *function);

struct obj_upvalue *obj_upvalue_new(struct vm *, size_t location);

struct obj_native_function *obj_native_function_new(struct vm *,
						    native_function_t *native_function);

struct obj_str_literal *obj_str_literal_new(struct vm *,
					    uint32_t str_literal_id);


#endif
