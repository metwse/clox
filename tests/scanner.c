#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/scanner.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


void test_input(struct scanner *s,
		const char *input,
		const enum token_type *types,
		const union seminfo *seminfos,
		size_t len)
{
	scanner_feed(s, input);

	struct token tk;
	for (size_t i = 0; i < len; i++) {
		tk = scanner_xnext(s);

		clox_assert(tk.ty == types[i],
			    "token type missmatch");

		if (tk.ty == TK_IDENT)
			clox_assert(tk.seminfo.ident_id == seminfos[i].ident_id,
				    "ident_id missmatch");

		if (tk.ty == TK_NUM)
			clox_assert(tk.seminfo.num == seminfos[i].num,
				    "num missmatch");

		if (tk.ty == TK_STR) {
			clox_assert(strcmp(tk.seminfo.str, seminfos[i].str) == 0,
				    "str missmatch");
			free(tk.seminfo.str);
		}
	}

	tk = scanner_xnext(s);
	clox_assert(tk.ty == TK_EOF, "still has tokens");
}


int main(void)
{
	scanner_xstatic_init();

	struct scanner s;
	scanner_xinit(&s);

	test_input(&s,
		   "    test  123 test2 < =  \n test   ; 321.123 >= >",
		   (enum token_type[]) {
			TK_IDENT, TK_NUM, TK_IDENT, TK_LT, TK_EQ, TK_IDENT,
			TK_SEMI, TK_NUM, TK_GT_EQ, TK_GT, TK_EOF
		   },
		   (union seminfo[]) {
			[0] = { .ident_id = 0 },
			[1] = { .num = 123 },
			[2] = { .ident_id = 1 },
			[5] = { .ident_id = 0 },
			[7] = { .num = 321.123 },
		   },
		   11);

	clox_assert(strcmp(scanner_get_ident_name(&s, 0), "test") == 0 &&
		    strcmp(scanner_get_ident_name(&s, 1), "test2") == 0,
		    "cannot get ident name by id");

	test_input(&s,
		   "    valid if ınvalıd ",
		   (enum token_type[]) {
			TK_IDENT, TK_IF, TK_INVALID
		   },
		   (union seminfo[]) {
			[0] = { .ident_id = 2 },
		   },
		   3);

	test_input(&s,
		   " \"string\" \"\\\"\" \"\" \"\\\\\" \"\\\\\\\"\"",
		   (enum token_type[]) {
			TK_STR, TK_STR, TK_STR, TK_STR, TK_STR
		   },
		   (union seminfo[]) {
			{ .str = "string" },
			{ .str = "\"" },
			{ .str = "" },
			{ .str = "\\" },
			{ .str = "\\\"" },
		   },
		   5);


	test_input(&s,
		   "\"unterminated string ",
		   (enum token_type[]) {
			TK_INVALID
		   },
		   NULL,
		   1);

	test_input(&s,
		   "\"invalid escape sequence \\a ",
		   (enum token_type[]) {
			TK_INVALID
		   },
		   NULL,
		   1);

	scanner_destroy(&s);

	scanner_static_destroy();
}
