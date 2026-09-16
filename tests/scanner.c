#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/scanner.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>


void test_input(struct scanner *s,
		const char *input,
		const enum tk_id *ids,
		const union seminfo_data *seminfos,
		size_t len)
{
	scanner_feed(s, input);

	enum tk_id tk_id;
	struct seminfo seminfo;

	for (size_t i = 0; i < len; i++) {
		scanner_xnext(s, &tk_id, &seminfo);

		clox_assert(tk_id == ids[i],
			    "token type missmatch");

		if (tk_id == TK_IDENT)
			clox_assert(seminfo.seminfo.ident_id == seminfos[i].ident_id,
				    "ident_id missmatch");

		if (tk_id == TK_NUM)
			clox_assert(seminfo.seminfo.num == seminfos[i].num,
				    "num missmatch");

		if (tk_id == TK_STR) {
			clox_assert(strcmp(seminfo.seminfo.str, seminfos[i].str) == 0,
				    "str missmatch");
			free(seminfo.seminfo.str);
		}
	}

	scanner_xnext(s, &tk_id, &seminfo);
	clox_assert(tk_id == TK_EOF || tk_id == TK_INVALID, "still has tokens");
}


int main(void)
{
	scanner_xstatic_init();

	struct scanner s;
	scanner_xinit(&s);

	test_input(&s,
		   "    test  123 test2 < =  \n test   ; 321.123 >= >",
		   (enum tk_id[]) {
			TK_IDENT, TK_NUM, TK_IDENT, TK_LT, TK_EQ, TK_IDENT,
			TK_SEMI, TK_NUM, TK_GT_EQ, TK_GT, TK_EOF
		   },
		   (union seminfo_data[]) {
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
		   (enum tk_id[]) {
			TK_IDENT, TK_IF, TK_INVALID
		   },
		   (union seminfo_data[]) {
			[0] = { .ident_id = 2 },
		   },
		   3);

	test_input(&s,
		   " \"string\" \"\\\"\" \"\" \"\\\\\" \"\\\\\\\"\"",
		   (enum tk_id[]) {
			TK_STR, TK_STR, TK_STR, TK_STR, TK_STR
		   },
		   (union seminfo_data[]) {
			{ .str = "string" },
			{ .str = "\"" },
			{ .str = "" },
			{ .str = "\\" },
			{ .str = "\\\"" },
		   },
		   5);


	test_input(&s,
		   "\"unterminated string ",
		   (enum tk_id[]) {
			TK_INVALID
		   },
		   NULL,
		   1);

	scanner_new_line(&s);

	test_input(&s,
		   "\"invalid escape sequence \\a ",
		   (enum tk_id[]) {
			TK_INVALID
		   },
		   NULL,
		   1);

	scanner_new_line(&s);

	scanner_destroy(&s);

	scanner_static_destroy();
}
