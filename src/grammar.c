#include "../include/grammar.h"

#include "../vendor/rdesc/include/rule_macros.h"


const char *const tk_names[TK_COUNT] = {
	"(", ")", "{", "}", ",", ".", "-", "+", ";", "/", "*",

	"!", "!=", "=", "==", ">", ">=", "<", "<=",

	"@IDENT", "@STR", "@NUM",

	"and", "class", "else", "false", "fun", "if", "nil", "or",
	"print", "return", "super", "this", "true", "var", "while",

	"@eof", "@invalid"
};

const char *const nt_names[NT_COUNT] = {
	"expression",
	"equality", "equality-rest", "equality-op",
	"comparison", "comparison-rest", "comparison-op",
	"term", "term-rest", "term-op",
	"factor", "factor-rest", "factor-op",
	"unary", "unary-op",
	"primary"
};

const struct rdesc_grammar_symbol production_rules
	[NT_COUNT][NT_MAX_ALTERNATIVE_COUNT + 1][NT_MAX_ALTERNATIVE_SIZE + 1] = {
/* <expression> ::= */ r(
	NT(EQUALITY)
),

/* <equality> ::= */
	rrr(EQUALITY, (NT(COMPARISON)), (NT(EQUALITY_OP), NT(COMPARISON))),
/* <equality-op> ::= */ r(
	TK(EXCL_EQ)
alt	TK(EQ_EQ)
),

/* <comparison> ::= */
	rrr(COMPARISON, (NT(TERM)), (NT(COMPARISON_OP), NT(TERM))),
/* <comparison-op> ::= */ r(
	TK(GT)
alt	TK(GT_EQ)
alt	TK(LT)
alt	TK(LT_EQ)
),

/* <term> ::= */
	rrr(TERM, (NT(FACTOR)), (NT(TERM_OP), NT(FACTOR))),
/* <term-op> ::= */ r(
	TK(MINUS)
alt	TK(PLUS)
),

/* <factor> ::= */
	rrr(FACTOR, (NT(UNARY)), (NT(FACTOR_OP), NT(UNARY))),
/* <factor-op> ::= */ r(
	TK(SLASH)
alt	TK(STAR)
),

/* <unary> ::= */ r(
	NT(UNARY_OP), NT(UNARY)
alt	NT(PRIMARY)
),
/* <unary-op> ::= */ r(
	TK(PLUS)
alt 	TK(MINUS)
),

/* <primary> ::= */ r(
	TK(NUM)
alt	TK(STR)
alt	TK(TRUE)
alt	TK(FALSE)
alt	TK(NIL)
alt	TK(LPAREN), NT(EXPRESSION), TK(RPAREN)
)
};
