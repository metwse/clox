#ifndef CLOX_H
#define CLOX_H

#include <stdbool.h>


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
