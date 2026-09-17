#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "../vendor/rdesc/include/grammar.h"


#define TK_COUNT 40

#define NT_COUNT 40
#define NT_MAX_ALTERNATIVE_COUNT 10
#define NT_MAX_ALTERNATIVE_SIZE 11

enum tk_id {
	/* Single-char tokens. */
	TK_LPAREN, TK_RPAREN, TK_LBRACE, TK_RBRACE,
	TK_COMMA, TK_DOT, TK_MINUS, TK_PLUS, TK_SEMI, TK_SLASH, TK_STAR,

	/* One or two char tokens. */
	TK_EXCL, TK_EXCL_EQ,
	TK_EQ, TK_EQ_EQ,
	TK_GT, TK_GT_EQ,
	TK_LT, TK_LT_EQ,

	/* Literals. */
	TK_IDENT, TK_STR, TK_NUM,

	/* Keywords. */
	TK_AND, TK_CLASS, TK_ELSE, TK_FALSE, TK_FOR, TK_FUN, TK_IF, TK_NIL,
	TK_OR, TK_PRINT, TK_RETURN, TK_SUPER, TK_THIS, TK_TRUE, TK_VAR,
	TK_WHILE,

	TK_EOF, TK_INVALID
};

enum nt_id {
	NT_DECL,
	NT_CLASS_DECL, NT_FUN_DECL, NT_VAR_DECL,
	NT_STMT,

	NT_EXPR_STMT,
	NT_FOR_STMT, NT_FOR_STMT_DECL,
	NT_IF_STMT, NT_IF_OPTELSE_STMT,
	NT_PRINT_STMT,
	NT_RETURN_STMT,
	NT_WHILE_STMT,
	NT_BLOCK, NT_BLOCK_DECLS,

	NT_EXPRESSION, NT_OPTEXPRESSION,
	NT_EQUALITY, NT_EQUALITY_REST, NT_EQUALITY_OP,
	NT_COMPARISON, NT_COMPARISON_REST, NT_COMPARISON_OP,
	NT_TERM, NT_TERM_REST, NT_TERM_OP,
	NT_FACTOR, NT_FACTOR_REST, NT_FACTOR_OP,
	NT_UNARY, NT_UNARY_OP,
	NT_PRIMARY,

	NT_CLASS_DECL_OPTINHERITANCE, NT_CLASS_DECL_FUNCTIONS,
	NT_VAR_DECL_OPTASGN,
	NT_FUNCTION,
	NT_FUNCTION_PARAMS, NT_FUNCTION_PARAMS_REST,
	NT_FUNCTION_ARGS, NT_FUNCTION_ARGS_REST,
};

union seminfo_data {
	int ident_id;
	double num;
	char *str;
};

struct seminfo {
	union seminfo_data seminfo;

	int line;
	int col;
};

extern const char *const tk_names[TK_COUNT];

extern const char *const nt_names[NT_COUNT];

extern const struct rdesc_grammar_symbol production_rules
	[NT_COUNT][NT_MAX_ALTERNATIVE_COUNT + 1][NT_MAX_ALTERNATIVE_SIZE + 1];

void token_destroyer(uint16_t, void *);


#endif
