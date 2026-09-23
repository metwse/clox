#include "globals_internal.h"

#include "../include/builtin_functions.h"
#include "../include/common.h"
#include "../include/globals.h"
#include "../include/grammar.h"
#include "../include/interpreter.h"
#include "../include/scanner.h"
#include "../include/string_pool.h"

#include "../vendor/rdesc/include/grammar.h"
#include "../vendor/rdesc/include/rdesc.h"

#include <stdint.h>
#include <string.h>


struct rdesc_grammar clox;

void interpreter_xstatic_init(void)
{
	scanner_xstatic_init();

	clox_assert(rdesc_grammar_init_checked(&clox,
					       NT_COUNT,
					       NT_MAX_ALTERNATIVE_COUNT,
					       NT_MAX_ALTERNATIVE_SIZE,
					       production_rules) == 0,
		   "cannot initialize grammar");
}

void interpreter_static_destroy(void)
{
	scanner_static_destroy();

	rdesc_grammar_destroy(&clox);
}


void interpreter_xinit(struct interpreter *i)
{
	clox_assert(rdesc_init(&i->parser,
			       &clox,
			       sizeof(struct seminfo),
			       NULL) == 0,
		   "cannot initialize the parser");

	str_pool_xinit(&i->strings);

	globals_xinit(&i->globals);

	scanner_xinit(&i->scanner, &i->strings);

	vm_xinit(&i->vm, &i->strings, &i->globals);

	clox_assert(rdesc_start(&i->parser, NT_DECL) == 0,
		    "cannot start rdesc");

	for (size_t j = 0; j < builtin_functions_len; j++) {
		struct builtin_function builtin_function = builtin_functions[j];

		uint32_t str_id = str_pool_xget_id(&i->strings,
						   builtin_function.name,
						   strlen(builtin_function.name));
		uint32_t global_id = globals_xget_global_id(&i->globals, str_id);

		struct obj_native_function *native_function =
			obj_native_function_new(builtin_function.function);

		globals_define(&i->globals,
			       global_id,
			       OBJ_VAL((struct obj *) native_function));
	}
}

void interpreter_destroy(struct interpreter *i)
{
	vm_destroy(&i->vm);

	str_pool_destroy(&i->strings);

	globals_destroy(&i->globals);

	rdesc_destroy(&i->parser);
}

int interpreter_run(struct interpreter *i, const char *source)
{
	scanner_feed(&i->scanner, source);

	enum tk_id tk_id = TK_EOF;
	struct seminfo seminfo;

	while (true) {
		enum rdesc_result res;
		if ((res = rdesc_resume(&i->parser)) == RDESC_CONTINUE) {
			scanner_xnext(&i->scanner, &tk_id, &seminfo);

			if (tk_id == TK_EOF)
				break;

			res = rdesc_pump(&i->parser, tk_id, &seminfo);
		}

		switch (res) {
		case RDESC_CONTINUE:
			continue;

		case RDESC_READY: {
			struct chunk chunk =
				chunk_xcompile(rdesc_get_root(&i->parser),
					       &i->globals);

			chunk_disassemble(&chunk, stderr, 0, 0);

			if (vm_execute(&i->vm, &chunk))
				clox_report("execution interrupted due to a "
					    "runtime error");

			chunk_destroy(&chunk);

			clox_assert(rdesc_start(&i->parser, NT_DECL) == 0,
				    "cannot start rdesc");
			continue;
		  }

		case RDESC_NOMATCH:
			rdesc_reset(&i->parser);

			clox_report("invalid token %s at line %d, column %d",
				    tk_names[tk_id], seminfo.line, seminfo.col);

			scanner_new_line(&i->scanner);

			clox_assert(rdesc_start(&i->parser, NT_DECL) == 0,
				    "cannot start rdesc");
			return 1;

		case RDESC_ENOMEM:
			clox_fatal("out of memory");
		}

	}

	return 0;
}
