#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "scanner.h"
#include "string_pool.h"
#include "vm.h"

#include "../vendor/rdesc/include/rdesc.h"


/* The clox interpreter. */
struct interpreter {
	struct str_pool idents;
	struct str_pool str_literals;
	struct scanner scanner;
	struct rdesc parser;
	struct vm vm;
};


/* Initializes all global data structures. */
void interpreter_xstatic_init(void);

/* Uninitializes and frees global data structures. */
void interpreter_static_destroy(void);

/* Initializes a new interpreter. */
void interpreter_xinit(struct interpreter *);

/* Frees resources owned by the interpreter. */
void interpreter_destroy(struct interpreter *);

/* Interprets the text source.
 * Returns non-zero in case of a failure. */
int interpreter_run(struct interpreter *, const char *source);


#endif
