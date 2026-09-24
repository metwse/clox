#ifndef COMPILER_INTERNAL
#define COMPILER_INTERNAL

#include "../include/chunk.h"

#include <stdint.h>


#define arg_u24(num) &*((uint8_t[3]) { \
			(num) & 255, \
			((num) >> 8) & 255, \
			((num) >> 16) & 255, \
		})

#define arg_u8or24(num) ((const char *) (num < 256 ? \
				&(uint8_t) { (uint8_t) num } : \
				arg_u24(num))) \

#define emit_const(v) do { \
		uint32_t constant_id = chunk_xpush_constant(c, &v); \
		emit_inst_u8or24(OP_CONSTANT, constant_id); \
	} while (0)

#define overwrite_inst(offset, opcode, arguments) \
	chunk_overwrite_inst(c, offset, \
			    (struct inst) { .op = opcode, .args = (arguments) });

#define emit_insts(opcode, opcode2) do { \
		emit_inst(opcode); emit_inst(opcode2); \
	} while (0)
#define emit_inst(opcode) \
	chunk_xwrite_inst(c, current->line, \
			(struct inst) { .op = opcode, .args = NULL })
#define emit_inst_args(opcode, arguments) \
	chunk_xwrite_inst(c, current->line, \
			(struct inst) { .op = opcode, .args = (arguments) })
#define emit_inst_u8or24(opcode, num) do { \
		emit_inst_args(num < 256 ? \
				opcode : opcode + 1, \
				(void *) arg_u8or24(num)); \
	} while (0)
#define emit_inst_u8(opcode, num) do { \
		emit_inst_args(opcode, &(uint8_t) { num }); \
	} while (0)

#define emit_inst_set(str_id) \
	compiler_emit_variable_inst(current, c, true, str_id)
#define emit_inst_get(str_id) \
	compiler_emit_variable_inst(current, c, false, str_id)

#define update_line(tk) do { \
		current->line = ((struct seminfo *) rseminfo(tk))->line; \
	} while (0)


struct local {
	uint32_t str_id;
	int depth;
	bool is_captured;
};

struct upvalue {
	uint32_t index;
	bool is_local;
};

#define T struct local, locals
#include "../vendor/libfun/include/stack.h"

#define T struct upvalue, upvalues
#include "../vendor/libfun/include/stack.h"


struct compiler {
	struct compiler *enclosing;
	struct vm *vm;
	struct fstack_upvalues upvalues;
	struct fstack_locals locals;
	int scope_depth;
	int line;
};


void chunk_xinit(struct chunk *);

/* Shrinks dynamic arrays of chunk as much as possible. */
void chunk_compact(struct chunk *);

void compiler_xinit(struct compiler *current,
		    struct compiler *enclosing,
		    struct vm *);

void compiler_destroy(struct compiler *);

/* Emits GET/SET instructions for local variables, upvalues, or globals. */
void compiler_emit_variable_inst(struct compiler *,
				 struct chunk *,
				 bool is_set,
				 uint32_t str_id);
/* Defines a new local variable. */
void compiler_emit_define_variable_inst(struct compiler *,
					struct chunk *,
					uint32_t str_id);

/* Emits closure a closure bytecode. */
void compiler_emit_closure_inst(struct compiler *current,
				struct compiler *enclosed,
				struct chunk *c,
				uint32_t constant_id);

/* Defines a new local variable. */
void compiler_define_local(struct compiler *, uint32_t str_id);

/* Creates a new scope. */
void compiler_begin_scope(struct compiler *);

/* Deletes a scope. */
void compiler_end_scope(struct compiler *, struct chunk *);


#endif
