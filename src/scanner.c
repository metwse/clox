#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/scanner.h"

#include "../vendor/libfun/include/hashmap.h"
#include "../vendor/libfun/include/stack.h"

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>


static struct fhashmap keyword_map;


void scanner_xstatic_init(void)
{
	fhashmap_xinit(&keyword_map, sizeof(enum token_type));

	for (enum token_type i = TK_AND; i <= TK_WHILE; i++)
		fhashmap_xinsert(&keyword_map,
				 keyword_names[i - TK_AND],
				 &(enum token_type) { i });
}

void scanner_static_destroy(void)
{
	fhashmap_destroy(&keyword_map);
}

void scanner_xinit(struct scanner *s)
{
	s->cur = NULL;

	fhashmap_xinit(&s->ident_id_map, sizeof(size_t));
	fstack_xinit(&s->ident_id_rev_map, sizeof(char *));

	s->last_id = -1;
	s->line = s->col = 1;
}

void scanner_destroy(struct scanner *s)
{
	fhashmap_destroy(&s->ident_id_map);

	for (size_t i = 0; i < fstack_len(&s->ident_id_rev_map); i++) {
		char *ident_name = *(char **) fstack_at(&s->ident_id_rev_map, i);
		free(ident_name);
	}
	fstack_destroy(&s->ident_id_rev_map);
}

void scanner_feed(struct scanner *s, const char *buf)
{
	s->cur = buf;
}

const char *scanner_get_ident_name(const struct scanner *s, size_t id)
{
	return *(const char **) fstack_at((struct fstack *) &s->ident_id_rev_map, id);
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

#define token(ty_id) \
	(struct token) { \
		.ty = ty_id, \
		.col = s->col - 1, \
		.line = s->line \
	}

#define invalid_token do { \
		s->cur = NULL; \
		return (struct token) { \
			.ty = TK_INVALID, \
			.col = s->col - 1, \
			.line = s->line \
		}; \
	} while (0)


/* Token collecting functions. */
static struct token collect_num(struct scanner *s)
{
	const char *start = s->cur;
	size_t len = 0;

	while (isdigit(peek(s))) {
		advance(s);
		len++;
	}

	if (peek(s) == '.' && isdigit(peek_next(s))) {
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

	double num = strtod(num_str, NULL);

	struct token tk = token(TK_NUM);
	tk.seminfo.num = num;

	return tk;
}

static struct token collect_ident_or_keyword(struct scanner *s)
{
	const char *start = s->cur;
	size_t ident_len = 0;

	while (isalnum(peek(s)) || peek(s) == '_') {
		ident_len++;
		advance(s);
	}

	enum token_type *keyword_id_ptr = fhashmap_get2(&keyword_map, start, ident_len);
	if (keyword_id_ptr)
		return token(*keyword_id_ptr);

	struct token tk = token(TK_IDENT);

	size_t *ident_id = fhashmap_get2(&s->ident_id_map, start, ident_len);
	if (ident_id == NULL) {
		size_t new_ident_id = ++s->last_id;

		fhashmap_xinsert2(&s->ident_id_map,
				  start,
				  ident_len,
				  &new_ident_id);

		char *ident_name = malloc(ident_len + 1);
		clox_assert(ident_name, "memory allocation for ident_name");
		ident_name[ident_len] = '\0';
		memcpy(ident_name, start, ident_len);

		fstack_xpush(&s->ident_id_rev_map, &ident_name);

		tk.seminfo.ident_id = new_ident_id;
	} else {
		tk.seminfo.ident_id = *ident_id;
	}

	return tk;
}

static struct token collect_punct(struct scanner *s)
{
	char c = peek(s);

	for (enum token_type i = TK_LPAREN; i <= TK_STAR; i++) {
		if (c == punct[i][0]) {
			advance(s);
			return token(i);
		}
	}

	for (enum token_type i = TK_EXCL_EQ; i <= TK_LT_EQ; i += 2) {
		if (c == punct[i][0]) {
			advance(s);
			if (peek(s) == punct[i][1]) {
				advance(s);
				return token(i);
			} else {
				return token(i - 1);
			}
		}
	}

	invalid_token;
}

struct token collect_str(struct scanner *s)
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
				invalid_token;
			}
		} else {
			advance(s);
			str_len++;
		}
	}

	if (peek(s) == '"') {
		advance(s);  /* consume the second " */

		char *str = malloc(str_len + 1);
		clox_assert(str, "memory allocation for str");
		str[str_len] = '\0';

		for (size_t i = 0; i < str_len; i++) {
			if (*start == '\\')
				start++;

			str[i] = *start;
			start++;
		}

		struct token tk = token(TK_STR);
		tk.seminfo.str = str;

		return tk;
	}

	invalid_token;
}

struct token scanner_xnext(struct scanner *s)
{
	while (isspace(peek(s)))
		advance(s);

	if (is_at_end(s))
		return token(TK_EOF);

	if (isdigit(peek(s)))
		return collect_num(s);

	if (isalnum(peek(s)) || peek(s) == '_')
		return collect_ident_or_keyword(s);

	if (peek(s) == '"')
		return collect_str(s);

	return collect_punct(s);
}
