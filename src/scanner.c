#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/scanner.h"
#include "../include/string_pool.h"

#define T char, enum tk_id, keywords
#include "../vendor/libfun/include/hmap.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


static struct fhmap_keywords keywords;


void scanner_xstatic_init(void)
{
	fhmap_keywords_xinit(&keywords);

	/* strlen excludes null-terminator. the scanner will pass non-null
	 * terminated string views, so we cannot use xinsert3 here as it adds
	 * null-terminator to keys */
	for (enum tk_id i = TK_BREAK; i <= TK_WHILE; i++)
		fhmap_keywords_xinsert(&keywords,
				       tk_names[i],
				       strlen(tk_names[i]),
				       &(enum tk_id) { i });

	fhmap_keywords_shrink_to_fit(&keywords);
}

void scanner_static_destroy(void)
{
	fhmap_keywords_destroy(&keywords);
}

void scanner_xinit(struct scanner *s, struct str_pool *strings)
{
	s->cur = NULL;

	s->strings = strings;

	s->line = s->col = 1;
}

void scanner_feed(struct scanner *s, const char *buf)
{
	s->cur = buf;
}

void scanner_new_line(struct scanner *s)
{
	s->cur = NULL;
	s->line++;
	s->col = 1;
}

/* Regex helpers. */
static bool is_at_end(struct scanner *s)
{
	return s->cur == NULL || *s->cur == '\0';
}

static char peek(struct scanner *s)
{
	return is_at_end(s) ? '\0' : *s->cur;
}

static char peek_next(struct scanner *s)
{
	return is_at_end(s) ? '\0' : *(s->cur + 1);
}

static void advance(struct scanner *s)
{
	if (peek(s) == '\n') {
		s->line++;
		s->col = 1;
	} else {
		s->col++;
	}

	s->cur++;
}

#define return_tk(id) do { \
		*out_id = id; \
		out_seminfo->line = s->line; \
		out_seminfo->col = s->col; \
		return; \
	} while (0)

/* Token collecting functions. */
static void collect_numeric(struct scanner *s,
			    enum tk_id *out_id,
			    struct seminfo *out_seminfo)
{
	const char *start = s->cur;
	size_t len = 0;
	bool is_float = false;

	while (isdigit(peek(s))) {
		advance(s);
		len++;
	}

	if (peek(s) == '.' && isdigit(peek_next(s))) {
		is_float = true;
		advance(s);
		len++;

		while (isdigit(peek(s))) {
			advance(s);
			len++;
		}
	}

	char num_str[len + 1];
	num_str[len] = '\0';
	memcpy(num_str, start, len);

	if (is_float) {
		out_seminfo->seminfo.number = Lw_str2number(num_str);
		return_tk(TK_NUMBER);
	} else {
		out_seminfo->seminfo.integer = Lw_str2integer(num_str);
		return_tk(TK_INTEGER);
	}
}

static void collect_ident_or_keyword(struct scanner *s,
				     enum tk_id *out_id,
				     struct seminfo *out_seminfo)
{
	const char *start = s->cur;
	size_t ident_len = 0;

	while (isalnum(peek(s)) || peek(s) == '_') {
		ident_len++;
		advance(s);
	}

	const enum tk_id *keyword_id_ptr =
		fhmap_keywords_get(&keywords, start, ident_len);
	if (keyword_id_ptr)
		return_tk(*keyword_id_ptr);

	uint32_t str_id = str_pool_xget_id(s->strings, start, ident_len);
	out_seminfo->seminfo.str_id = str_id;

	return_tk(TK_IDENT);
}

static void collect_punct(struct scanner *s,
			  enum tk_id *out_id,
			  struct seminfo *out_seminfo)
{
	char c = peek(s);

	for (enum tk_id i = TK_LBRACE; i <= TK_STAR; i++) {
		if (c == tk_names[i][0]) {
			advance(s);

			return_tk(i);
		}
	}

	for (enum tk_id i = TK_EXCL_EQ; i <= TK_PIPE_PIPE; i += 2) {
		if (c == tk_names[i][0]) {
			advance(s);
			if (peek(s) == tk_names[i][1]) {
				advance(s);

				return_tk(i);
			} else {
				return_tk(i - 1);
			}
		}
	}

	for (enum tk_id i = TK_AND_AND; i <= TK_AND_AND; i++) {
		if (c == tk_names[i][0]) {
			advance(s);
			if (peek(s) == tk_names[i][1]) {
				advance(s);

				return_tk(i);
			}
		}
	}

	return_tk(TK_INVALID);
}

static void collect_str(struct scanner *s,
			enum tk_id *out_id,
			struct seminfo *out_seminfo)
{
	advance(s);  /* consume the " */

	const char *start = s->cur;
	size_t str_len = 0;

	while (peek(s) != '"' && !is_at_end(s)) {
		if (peek(s) == '\\') {
			if (peek_next(s) == '"' || peek_next(s) == '\\') {
				advance(s);
				advance(s);
				str_len++;
			} else {
				return_tk(TK_INVALID);
			}
		} else {
			advance(s);
			str_len++;
		}
	}

	if (peek(s) == '"') {
		advance(s);  /* consume the second " */

		char escaped_str[str_len];
		size_t escaped_str_len;

		for (escaped_str_len = 0;
		     escaped_str_len < str_len;
		     escaped_str_len++) {
			if (*start == '\\')
				start++;

			escaped_str[escaped_str_len] = *start;
			start++;
		}

		uint32_t str_literal_id = str_pool_xget_id(s->strings,
							   escaped_str,
							   escaped_str_len);
		out_seminfo->seminfo.str_id = str_literal_id;
		return_tk(TK_STR);
	}

	return_tk(TK_INVALID);
}

void scanner_xnext(struct scanner *s,
		   enum tk_id *out_id,
		   struct seminfo *out_seminfo)
{
	while (isspace(peek(s)))
		advance(s);

	if (is_at_end(s))
		return_tk(TK_EOF);
	else if (isdigit(peek(s)))
		collect_numeric(s, out_id, out_seminfo);
	else if (isalnum(peek(s)) || peek(s) == '_')
		collect_ident_or_keyword(s, out_id, out_seminfo);
	else if (peek(s) == '"')
		collect_str(s, out_id, out_seminfo);
	else collect_punct(s, out_id, out_seminfo);
}
