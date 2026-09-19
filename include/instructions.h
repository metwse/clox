#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


#define LAST_OPCODE OP_NOT

enum opcode {
	OP_RETURN,
	OP_PRINT,
	OP_POP,

	OP_CONSTANT, OP_CONSTANT_LONG,
	OP_CONSTANT_ONCE, OP_CONSTANT_ONCE_LONG,

	OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_LONG,
	OP_GET_GLOBAL, OP_GET_GLOBAL_LONG,
	OP_SET_GLOBAL, OP_SET_GLOBAL_LONG,
	OP_GET_LOCAL, OP_GET_LOCAL_LONG,
	OP_SET_LOCAL, OP_SET_LOCAL_LONG,

	OP_JUMP, OP_JUMP_IF_FALSE,
	OP_JUMP_BACK,

	OP_CALL,

	OP_NIL,
	OP_TRUE,
	OP_FALSE,

	OP_EQUAL,
	OP_GREATER,
	OP_LESS,

	OP_ADD,
	OP_SUBSTRACT,
	OP_MULTIPLY,
	OP_DIVIDE,

	OP_NEGATE,
	OP_AND,
	OP_OR,
	OP_NOT,
};

static const char *const opcode_names[LAST_OPCODE + 1] = {
	"RETURN",
	"PRINT",
	"POP",

	"CONSTANT", "CONSTANT_LONG",
	"CONSTANT_ONCE", "CONSTANT_ONCE_LONG",

	"DEFINE_GLOBAL", "DEFINE_GLOBAL_LONG",
	"GET_GLOBAL", "GET_GLOBAL_LONG",
	"SET_GLOBAL", "SET_GLOBAL_LONG",
	"GET_LOCAL", "GET_LOCAL_LONG",
	"SET_LOCAL", "SET_LOCAL_LONG",

	"JUMP", "JUMP_IF_FALSE",
	"JUMP_BACK",

	"CALL",

	"NIL",
	"TRUE",
	"FALSE",

	"EQUAL",
	"GREATER",
	"LESS",

	"ADD",
	"SUBSTRACT",
	"MULTIPLY",
	"DIVIDE",

	"NEGATE",
	"AND",
	"OR",
	"NOT",
};

struct inst {
	enum opcode op;
	void *args;
};


/* Gets the total size of arguments. */
size_t inst_arg_len(enum opcode);

/* Debug printing an instruction. */
void inst_print(struct inst, FILE *out, int line);

/* Get one byte argument. */
uint8_t inst_get_u8_arg(struct inst, size_t offset);

/* Get 3 byte argument. */
uint32_t inst_get_u24_arg(struct inst, size_t offset);


#endif
