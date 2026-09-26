#ifndef VALUE_H
#define VALUE_H

#include "config.h"

#include <stdbool.h>


#define NUMBER_VAL(n) ((struct val) { .type = VAL_NUMBER, .val.number = n })
#define INTEGER_VAL(n) ((struct val) { .type = VAL_INTEGER, .val.integer = n })
#define BOOL_VAL(b) ((struct val) { .type = VAL_BOOL, .val.boolean = b })
#define UNIT_VAL ((struct val) { .type = VAL_UNIT })
#define OBJ_VAL(o) ((struct val) { .type = VAL_OBJ, .val.obj = o })

#define IS_NUMBER(v) ((v).type == VAL_NUMBER)
#define IS_INTEGER(v) ((v).type == VAL_INTEGER)
#define IS_BOOL(v) ((v).type == VAL_BOOL)
#define IS_UNIT(v) ((v).type == VAL_UNIT)
#define IS_OBJ(v) ((v).type == VAL_OBJ)

#define AS_NUMBER(v) ((v).type == VAL_NUMBER ? \
		(v).val.number : (Lw_number_t) (v).val.integer)
#define AS_INTEGER(v) ((v).val.integer)
#define AS_BOOL(v) ((v).val.boolean)
#define AS_OBJ(v) ((v).val.obj)


struct obj;  /* defined in object.h */


enum val_type {
	VAL_NUMBER,
	VAL_INTEGER,
	VAL_BOOL,
	VAL_UNIT,
	VAL_OBJ,
};

static const char *const val_names[] = {
	"number",
	"integer",
	"bool",
	"unit",
	"obj",
};

union val_data {
	Lw_number_t number;
	Lw_integer_t integer;
	bool boolean;
	struct obj *obj;
};

struct val {
	enum val_type type;

	union val_data val;
};


#define T struct val, vals
#include "../vendor/libfun/include/stack.h"


#endif
