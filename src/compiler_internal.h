#ifndef COMPILER_INTERNAL
#define COMPILER_INTERNAL

#include "../include/chunk.h"

#include "../vendor/libfun/include/stack.h"

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

#define emit_owned_const(v) do { \
		uint32_t constant_id = chunk_xpush_constant(c, &v); \
		emit_inst_u8or24(OP_CONSTANT_ONCE, constant_id); \
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
				opcode : opcode ## _LONG, \
				(void *) arg_u8or24(num)); \
	} while (0)
#define emit_inst_u8(opcode, num) do { \
		emit_inst_args(opcode, &(uint8_t) { num }); \
	} while (0)

#define update_line(tk) do { \
		current->line = ((struct seminfo *) rseminfo(tk))->line; \
	} while (0)


struct compiler {
	int line;
	struct fstack locals;
	int scope_depth;
};

struct local {
	uint32_t ident_id;
	int depth;
};


void compiler_xinit(struct compiler *);

void compiler_destroy(struct compiler *);

/* Returns slot of the local variable if found, UINT_MAX otherwise. */
uint32_t compiler_resolve_local(struct compiler *, uint32_t ident_id);

/* Defines a new local variable. */
void compiler_define_local(struct compiler *, uint32_t ident_id);

/* Creates a new scope. */
void compiler_begin_scope(struct compiler *);

/* Deletes a scope. */
void compiler_end_scope(struct compiler *, struct chunk *);

/* End scope without local variable cleanup. */
void compiler_end_scope_without_cleanup(struct compiler *);


#endif
