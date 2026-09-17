#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>


enum obj_type {
	OBJ_STRING,
};

struct obj {
	enum obj_type type;
};


struct obj_string {
	struct obj obj;
	size_t len;
	char *chars;
};


struct obj_string *obj_string_new(char *);


#endif
