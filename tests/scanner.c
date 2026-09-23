#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/scanner.h"
#include "../include/string_pool.h"

#include <stddef.h>
#include <stdint.h>
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
			clox_assert(seminfo.seminfo.str_literal_id == seminfos[i].str_literal_id,
				    "str missmatch");
		}
	}

	scanner_xnext(s, &tk_id, &seminfo);
	clox_assert(tk_id == TK_EOF || tk_id == TK_INVALID, "still has tokens");
}

void test_str_pool_id(struct str_pool *p, uint32_t id, const char *str)
{
	const char *out_chars;
	size_t out_len;

	str_pool_get_chars(p, id, &out_chars, &out_len);

	if (id == UINT32_MAX)
		clox_assert(out_len == 0,
			    "zero-length string should have id UINT32_MAX");
	else
		clox_assert(memcmp(str, out_chars, out_len) == 0,
			    "string pool missmatch");
}


int main(void)
{
	scanner_xstatic_init();

	struct str_pool strings;
	str_pool_xinit(&strings);

	struct scanner s;
	scanner_xinit(&s, &strings);

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

	test_str_pool_id(&strings, 0, "test");
	test_str_pool_id(&strings, 1, "test2");

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
		   " \"string\" \"\\\"\" \"\" \"\\\\\" \"\\\\\\\"\" \"string\"",
		   (enum tk_id[]) {
			TK_STR, TK_STR, TK_STR, TK_STR, TK_STR, TK_STR
		   },
		   (union seminfo_data[]) {
			{ .str_literal_id = 3 },
			{ .str_literal_id = 4 },
			{ .str_literal_id = UINT32_MAX },
			{ .str_literal_id = 5 },
			{ .str_literal_id = 6 },
			{ .str_literal_id = 3 },
		   },
		   6);

	test_str_pool_id(&strings, 3, "string");
	test_str_pool_id(&strings, 4, "\"");
	test_str_pool_id(&strings, UINT32_MAX, "");
	test_str_pool_id(&strings, 5, "\\");
	test_str_pool_id(&strings, 6, "\\\"");

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

	str_pool_destroy(&strings);

	scanner_static_destroy();
}
