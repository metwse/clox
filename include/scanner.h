#ifndef SCANNER_H
#define SCANNER_H

#include "common.h"
#include "grammar.h"

#include "../vendor/libfun/include/hashmap.h"
#include "../vendor/libfun/include/stack.h"

#include <stddef.h>


struct scanner {
	const char *cur;

	/* identifier name -> id map */
	struct fhashmap ident_id_map;
	/* identifier id -> name map */
	struct fstack ident_id_rev_map;

	size_t last_id;

	int line;
	int col;
};

/* Initializes global scanner data structures (e.g. keyword map) */
void scanner_xstatic_init(void);

/* Frees static scanner resources. */
void scanner_static_destroy(void);

/* Initializes a new scanner. */
void scanner_xinit(struct scanner *);

/* Release resources owned by the scanner. */
void scanner_destroy(struct scanner *);

/* Feed the scanner with a string input. */
void scanner_feed(struct scanner *, const char *buf);

/* Get name of the identifier by its ID. */
const char *scanner_get_ident_name(const struct scanner *, size_t id);

/* Consume the next token. Clears the scanner buffer and returns TK_EOF if an
 * invalid token is encountered. */
struct token scanner_xnext(struct scanner *) _wur;


#endif
