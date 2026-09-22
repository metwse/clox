#ifndef VALUE_H
#define VALUE_H

#include <stdbool.h>


#define NUM_VAL(n) ((struct val) { .type = VAL_NUM, .val.num = n })
#define BOOL_VAL(b) ((struct val) { .type = VAL_BOOL, .val.boolean = b })
#define NIL_VAL ((struct val) { .type = VAL_NIL })
#define OBJ_VAL(o) ((struct val) { .type = VAL_OBJ, .val.obj = o })

#define IS_NUM(v) (v.type == VAL_NUM)
#define IS_BOOL(v) (v.type == VAL_BOOL)
#define IS_NIL(v) (v.type == VAL_NIL)
#define IS_OBJ(v) (v.type == VAL_OBJ)

#define AS_NUM(v) (v.val.num)
#define AS_BOOL(v) (v.val.boolean)
#define AS_OBJ(v) (v.val.obj)


struct obj;  /* defined in object.h */


enum val_type {
	VAL_NUM,
	VAL_BOOL,
	VAL_NIL,
	VAL_OBJ,
};

union val_data {
	double num;
	bool boolean;
	struct obj *obj;
};

struct val {
	enum val_type type;

	union val_data val;
};


#define T struct val, val
#include "../vendor/libfun/include/stack.h"


#endif
