#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

struct str_pool;  /* defined in string_pool.h */


#define inst_get_u8_or_u24_arg(inst, offset) \
	(inst_arg_len(inst.op) == 3 ? \
	 	inst_get_u24_arg(inst, offset) : inst_get_u8_arg(inst, offset))


#define LAST_OPCODE OP_NOT

union op_closure_arg {
	uint8_t last : 1;

	struct op_closure_upvalue_arg {
		uint8_t _pad : 1;
		uint8_t is_local : 1;
		uint8_t is_long : 1;
		uint8_t index[];  /* 1 or 3 bytes */
	} upvalue;
};

#define DEF_U8OR24(name) name, name ## _LONG
enum opcode {
	OP_RETURN,
	OP_PRINT,
	OP_POP,
	OP_CLOSE_UPVALUE,

	DEF_U8OR24(OP_CONSTANT),
	DEF_U8OR24(OP_DEFINE_GLOBAL),
	DEF_U8OR24(OP_GET_GLOBAL),
	DEF_U8OR24(OP_SET_GLOBAL),
	DEF_U8OR24(OP_GET_LOCAL),
	DEF_U8OR24(OP_SET_LOCAL),
	DEF_U8OR24(OP_GET_UPVALUE),
	DEF_U8OR24(OP_SET_UPVALUE),

	OP_JUMP, OP_JUMP_IF_FALSE,
	OP_JUMP_BACK,

	DEF_U8OR24(OP_CLOSURE),
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
#undef DEF_U8OR24

#define DEF_U8OR24(name) name, name "_LONG"
static const char *const opcode_names[LAST_OPCODE + 1] = {
	"RETURN",
	"PRINT",
	"POP",
	"CLOSE_UPVALUE",

	DEF_U8OR24("CONSTANT"),
	DEF_U8OR24("DEFINE_GLOBAL"),
	DEF_U8OR24("GET_GLOBAL"),
	DEF_U8OR24("SET_GLOBAL"),
	DEF_U8OR24("GET_LOCAL"),
	DEF_U8OR24("SET_LOCAL"),
	DEF_U8OR24("GET_UPVALUE"),
	DEF_U8OR24("SET_UPVALUE"),

	"JUMP", "JUMP_IF_FALSE",
	"JUMP_BACK",

	DEF_U8OR24("CLOSURE"),
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
#undef DEF_U8OR24

struct inst {
	enum opcode op;
	void *args;
};

/* Gets the total size of the arguments. */
size_t inst_arg_len(enum opcode op);

/* Gets the total size of the instruction. */
size_t inst_len(struct inst);

/* Debug printing an instruction. */
void inst_print(struct inst,
		FILE *out,
		int line,
		const struct str_pool *strings);

/* Get one byte argument. */
uint8_t inst_get_u8_arg(struct inst, size_t offset);

/* Get 3 byte argument. */
uint32_t inst_get_u24_arg(struct inst, size_t offset);

/* Gets the next closure argument. Returns 0 if no arguments remaining, or the
 * argument length (1 or 3). */
int inst_get_next_closure_arg(struct inst,
			      size_t offset,
			      uint32_t *out_index,
			      bool *out_is_local);


#endif
