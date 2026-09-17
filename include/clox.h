#ifndef CLOX_H
#define CLOX_H

#include "object.h"

#include <stdbool.h>


#define NUM_VAL(n) ((struct clox_value) { .type = VAL_NUM, .value.num = n })
#define BOOL_VAL(b) ((struct clox_value) { .type = VAL_BOOL, .value.boolean = b })
#define NIL_VAL ((struct clox_value) { .type = VAL_NIL })
#define OBJ_VAL(o) ((struct clox_value) { .type = VAL_OBJ, .value.obj = o })

#define IS_NUM(v) (v.type == VAL_NUM)
#define IS_BOOL(v) (v.type == VAL_BOOL)
#define IS_NIL(v) (v.type == VAL_NIL)
#define IS_OBJ(v) (v.type == VAL_OBJ)

#define AS_NUM(v) (v.value.num)
#define AS_BOOL(v) (v.value.boolean)
#define AS_OBJ(v) (v.value.obj)

#define OBJ_TYPE(v) (AS_OBJ(v)->type)

#define AS_STRING(v) ((struct obj_string *) AS_OBJ(v))
#define AS_CSTRING(v) (((struct obj_string *) AS_OBJ(v))->chars)


enum clox_value_type {
	VAL_NUM,
	VAL_BOOL,
	VAL_NIL,
	VAL_OBJ,
};

union clox_value_data {
	double num;
	bool boolean;
	struct obj *obj;
};

struct clox_value {
	enum clox_value_type type;

	union clox_value_data value;
};


void obj_print(struct clox_value v);

bool obj_is_equal(struct clox_value a, struct clox_value b);

void obj_free(struct clox_value v);

static inline bool is_obj_type(struct clox_value v, enum obj_type t)
{
	return IS_OBJ(v) && OBJ_TYPE(v) == t;
}


#endif
