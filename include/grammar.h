#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "config.h"

#include "../vendor/rdesc/include/grammar.h"


#define TK_COUNT 52

#define NT_COUNT 46
#define NT_MAX_ALTERNATIVE_COUNT 10
#define NT_MAX_ALTERNATIVE_SIZE 11

enum tk_id {
	/* Single-char tokens. */
	TK_LBRACE, TK_RBRACE,
	TK_LPAREN, TK_RPAREN,
	TK_LSQ_BRACKET, TK_RSQ_BRACKET,
	TK_COLON, TK_COMMA, TK_DOT, TK_PLUS, TK_SEMI, TK_SLASH,
	TK_STAR,

	/* One or two char tokens. */
	TK_EXCL, TK_EXCL_EQ,
	TK_EQ, TK_EQ_EQ,
	TK_GT, TK_GT_EQ,
	TK_LT, TK_LT_EQ,
	TK_MINUS, TK_RARROW,
	TK_PIPE, TK_PIPE_PIPE,

	/* Two char tokens. */
	TK_AND_AND,

	/* Literals. */
	TK_IDENT, TK_STR, TK_NUMBER, TK_INTEGER,

	/* Keywords. */
	TK_BREAK, TK_CONTINUE, TK_ELSE, TK_ENUM, TK_FALSE, TK_FN, TK_FOR,
	TK_IF, TK_IMPL, TK_LET, TK_LOOP, TK_RETURN, TK_SELF, TK_SELF_TY,
	TK_STRUCT, TK_TRAIT, TK_TRUE, TK_TYPE, TK_WHERE, TK_WHILE,

	TK_EOF, TK_INVALID
};

enum nt_id {
	NT_DECL,
	NT_FN_DECL, NT_VAR_DECL,
	NT_STMT,

	NT_EXPR_STMT,
	NT_FOR_STMT, NT_FOR_STMT_DECL,
	NT_IF_STMT, NT_IF_OPTELSE_STMT,
	NT_RETURN_STMT,
	NT_WHILE_STMT,
	NT_BLOCK, NT_BLOCK_DECLS,

	NT_EXPRESSION, NT_OPTEXPRESSION,  /* expressions */
	NT_ASGN, NT_ASGN_OPTEQ,  /* = */
	NT_LOGIC_OR, NT_LOGIC_OR_REST,  /* or */
	NT_LOGIC_AND, NT_LOGIC_AND_REST,  /* and */
	NT_EQUALITY, NT_EQUALITY_REST, NT_EQUALITY_OP,  /* ==, != */
	NT_COMPARISON, NT_COMPARISON_REST, NT_COMPARISON_OP,  /* >, >=, <, <= */
	NT_TERM, NT_TERM_REST, NT_TERM_OP,  /* +, - */
	NT_FACTOR, NT_FACTOR_REST, NT_FACTOR_OP,  /* *, / */
	NT_UNARY, NT_UNARY_OP,
	NT_CALL, NT_CALL_OPTARGS_OR_GETATTR,
	NT_PRIMARY,

	NT_VAR_DECL_OPTASGN,
	NT_FN,
	NT_FN_PARAMS, NT_FN_PARAMS_REST, NT_FN_OPTPARAMS,
	NT_FN_ARGS, NT_FN_ARGS_REST, NT_FN_OPTARGS,
};

union seminfo_data {
	uint32_t str_id  /* TK_STR or TK_IDENT */;
	Lw_number_t number  /* TK_NUMBER */;
	Lw_integer_t integer  /* TK_INTEGER */;
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


#endif
