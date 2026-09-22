#include "../include/common.h"
#include "../include/grammar.h"
#include "../include/scanner.h"

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
	for (enum tk_id i = TK_AND; i <= TK_WHILE; i++)
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

void scanner_xinit(struct scanner *s)
{
	s->cur = NULL;

	fhmap_ident_ids_xinit(&s->ident_ids);
	fstack_ident_names_xinit(&s->ident_names);

	s->last_id = -1;
	s->line = s->col = 1;
}

void scanner_destroy(struct scanner *s)
{
	fhmap_ident_ids_destroy(&s->ident_ids);

	for (size_t i = 0;
	     i < fstack_ident_names_len(&s->ident_names);
	     i++) {
		char *ident_name =
			*fstack_ident_names_at_mut(&s->ident_names, i);
		free(ident_name);
	}
	fstack_ident_names_destroy(&s->ident_names);
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

const char *scanner_get_ident_name(const struct scanner *s, size_t id)
{
	return *fstack_ident_names_at(&s->ident_names, id);
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
static void collect_num(struct scanner *s,
			enum tk_id *out_id,
			struct seminfo *out_seminfo)
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

	out_seminfo->seminfo.num = num;
	return_tk(TK_NUM);
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
	if (keyword_id_ptr) {
		return_tk(*keyword_id_ptr);
	}

	const uint32_t *ident_id = fhmap_ident_ids_get(&s->ident_ids,
						       start,
						       ident_len);
	if (ident_id == NULL) {
		uint32_t new_ident_id = ++s->last_id;

		fhmap_ident_ids_xinsert(&s->ident_ids, start,
					ident_len,
					&new_ident_id);

		char *ident_name = malloc(ident_len + 1);
		clox_assert(ident_name, "memory allocation for ident_name");
		ident_name[ident_len] = '\0';
		memcpy(ident_name, start, ident_len);

		fstack_ident_names_xpush(&s->ident_names, &ident_name);

		out_seminfo->seminfo.ident_id = new_ident_id;
	} else {
		out_seminfo->seminfo.ident_id = *ident_id;
	}

	return_tk(TK_IDENT);
}

static void collect_punct(struct scanner *s,
			  enum tk_id *out_id,
			  struct seminfo *out_seminfo)
{
	char c = peek(s);

	for (enum tk_id i = TK_LPAREN; i <= TK_STAR; i++) {
		if (c == tk_names[i][0]) {
			advance(s);

			return_tk(i);
		}
	}

	for (enum tk_id i = TK_EXCL_EQ; i <= TK_LT_EQ; i += 2) {
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

		char *str = malloc(str_len + 1);
		clox_assert(str, "memory allocation for str");
		str[str_len] = '\0';

		for (size_t i = 0; i < str_len; i++) {
			if (*start == '\\')
				start++;

			str[i] = *start;
			start++;
		}

		out_seminfo->seminfo.str = str;
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
		collect_num(s, out_id, out_seminfo);
	else if (isalnum(peek(s)) || peek(s) == '_')
		collect_ident_or_keyword(s, out_id, out_seminfo);
	else if (peek(s) == '"')
		collect_str(s, out_id, out_seminfo);
	else collect_punct(s, out_id, out_seminfo);
}
