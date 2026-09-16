#include "../include/common.h"
#include "../include/grammar.h"

#include "../vendor/rdesc/include/grammar.h"
#include "../vendor/rdesc/include/util.h"

#include <stdio.h>  // IWYU pragma: keep. False positive unused header: stdout


int main(void)
{
	struct rdesc_grammar clox;

	clox_assert(rdesc_grammar_init_checked(&clox,
					       NT_COUNT,
					       NT_MAX_ALTERNATIVE_COUNT,
					       NT_MAX_ALTERNATIVE_SIZE,
					       production_rules) == 0,
		   "cannot initialize grammar");

	rdesc_dump_bnf(stdout, &clox, tk_names, nt_names);

	rdesc_grammar_destroy(&clox);
}
