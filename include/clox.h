#ifndef CLOX_H
#define CLOX_H

#include <stdbool.h>


#define NUM_VAL(n) ((struct clox_value) { .type = VAL_NUM, .value.num = n })
#define BOOL_VAL(b) ((struct clox_value) { .type = VAL_BOOL, .value.boolean = b })
#define NIL_VAL ((struct clox_value) { .type = VAL_NIL })

#define IS_NUM(v) (v.type == VAL_NUM)
#define IS_BOOL(v) (v.type == VAL_BOOL)
#define IS_NIL(v) (v.type == VAL_NIL)

#define AS_NUM(v) (v.value.num)
#define AS_BOOL(v) (v.value.boolean)

struct clox_value {
	enum clox_value_type {
		VAL_NUM,
		VAL_BOOL,
		VAL_NIL,
	} type;

	union clox_value_data {
		double num;
		bool boolean;
	} value;
};


#endif
