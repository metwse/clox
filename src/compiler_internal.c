#include "compiler_internal.h"

#include "../include/chunk.h"
#include "../include/common.h"
#include "../include/instructions.h"

#include <limits.h>
#include <stdint.h>


void compiler_xinit(struct compiler *current)
{
	current->line = 0;
	current->scope_depth = 0;
	fstack_xinit(&current->locals, sizeof(struct local));
}

void compiler_destroy(struct compiler *current)
{
	clox_assert(current->scope_depth == 0, "variables are not cleaned");

	fstack_destroy(&current->locals);
}

uint32_t compiler_resolve_local(struct compiler *current, uint32_t ident_id)
{
	for (size_t i = fstack_len(&current->locals); i > 0; i--) {
		struct local *var = fstack_at(&current->locals, i - 1);
		if (var->ident_id == ident_id)
			return i - 1;
	}

	return UINT_MAX;
}

void compiler_define_local(struct compiler *current, uint32_t ident_id)
{
	fstack_xpush(&current->locals,
		     &(struct local) {
			     .ident_id = ident_id,
			     .depth = current->scope_depth
		     });
}

void compiler_begin_scope(struct compiler *current)
{
	current->scope_depth++;
}

void compiler_end_scope(struct compiler *current, struct chunk *c)
{
	/* pop local variables */
	struct local *local;
	while (fstack_len(&current->locals) > 0 &&
	       (local = fstack_top(&current->locals)) &&
	       local->depth == current->scope_depth) {
		fstack_pop(&current->locals);
		emit_inst(OP_POP);
	}
	current->scope_depth--;
}

void compiler_end_scope_without_cleanup(struct compiler *current)
{
	current->scope_depth--;
}
