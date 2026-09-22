#ifndef SCANNER_H
#define SCANNER_H

#include "grammar.h"
#include "string_pool.h"

#include <stddef.h>

struct scanner {
	const char *cur;

	struct str_pool *idents;
	struct str_pool *str_literals;

	int line;
	int col;
};

/* Initializes global scanner data structures (e.g. keyword map) */
void scanner_xstatic_init(void);

/* Frees static scanner resources. */
void scanner_static_destroy(void);

/* Initializes a new scanner. */
void scanner_xinit(struct scanner *,
		   struct str_pool *idents,
		   struct str_pool *str_literals);

/* Feed the scanner with a string input. */
void scanner_feed(struct scanner *, const char *buf);

/* Reset scanner buffer and increment line number. */
void scanner_new_line(struct scanner *);

/* Consume the next token. Clears the scanner buffer and returns TK_EOF if an
 * invalid token is encountered. */
void scanner_xnext(struct scanner *,
		   enum tk_id *out_tk_id,
		   struct seminfo *out_seminfo);


#endif
