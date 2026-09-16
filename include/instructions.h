#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


enum opcode {
	OP_RETURN,
	OP_CONSTANT,
	OP_CONSTANT_LONG,
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
