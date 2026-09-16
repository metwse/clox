#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/interpreter.h"
#include "../include/scanner.h"

#include "../vendor/rdesc/include/grammar.h"
#include "../vendor/rdesc/include/rdesc.h"


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
			       token_destroyer) == 0,
		   "cannot initialize the parser");

	scanner_xinit(&i->scanner);

	vm_xinit(&i->vm);

	clox_assert(rdesc_start(&i->parser, NT_DECL) == 0,
		    "cannot start rdesc");
}

void interpreter_destroy(struct interpreter *i)
{
	rdesc_destroy(&i->parser);
	scanner_destroy(&i->scanner);
	vm_destroy(&i->vm);
}

int interpreter_run(struct interpreter *i, const char *source)
{
	scanner_feed(&i->scanner, source);

	struct chunk chunk;

	enum tk_id tk_id;
	struct seminfo seminfo;

	while (true) {
		scanner_xnext(&i->scanner, &tk_id, &seminfo);

		if (tk_id == TK_EOF)
			break;

		enum rdesc_result res = rdesc_pump(&i->parser, tk_id, &seminfo);

		switch (res) {
		case RDESC_CONTINUE:
			continue;

		case RDESC_READY:
			chunk_xinit(&chunk);

			chunk_xcompile(&chunk, rdesc_get_root(&i->parser));

			chunk_disassemble(&chunk, stderr, 0, 0);

			vm_set_chunk(&i->vm, &chunk);
			if (vm_run(&i->vm))
				clox_report("execution interrupted due to a "
					    "runtime error");

			chunk_destroy(&chunk);

			clox_assert(rdesc_start(&i->parser, NT_DECL) == 0,
				    "cannot start rdesc");
			continue;

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
