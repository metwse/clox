#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


#define LAST_OPCODE OP_NEGATE

enum opcode {
	OP_RETURN,

	OP_CONSTANT,
	OP_CONSTANT_LONG,

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
};

static const char *const opcode_names[] = {
	"RETURN",
	"CONSTANT",
	"CONSTANT_LONG",
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
char inst_get_char_arg(struct inst, size_t offset);

/* Get 3 byte argument. */
uint32_t inst_get_u24_arg(struct inst, size_t offset);


#endif
