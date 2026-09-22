#ifndef SCANNER_H
#define SCANNER_H

#include "grammar.h"

#include <stddef.h>


#define T char, uint32_t, ident_ids
#include "../vendor/libfun/include/hmap.h"

#define T char *, ident_names
#include "../vendor/libfun/include/stack.h"


struct scanner {
	const char *cur;

	/* identifier name -> id map */
	struct fhmap_ident_ids ident_ids;
	/* identifier id -> name map */
	struct fstack_ident_names ident_names;

	uint32_t last_id;

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

/* Reset scanner buffer and increment line number. */
void scanner_new_line(struct scanner *);

/* Get name of the identifier by its ID. */
const char *scanner_get_ident_name(const struct scanner *, size_t ident_id);

/* Consume the next token. Clears the scanner buffer and returns TK_EOF if an
 * invalid token is encountered. */
void scanner_xnext(struct scanner *,
		   enum tk_id *out_tk_id,
		   struct seminfo *out_seminfo);


#endif
