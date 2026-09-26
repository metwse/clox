#include "../include/grammar.h"

#include "../vendor/rdesc/include/rule_macros.h"


const char *const tk_names[TK_COUNT] = {
	"{", "}", "(", ")", "[", "]",
	":", ",", ".", "+", ";", "/", "*",

	"!", "!=", "=", "==", ">", ">=", "<", "<=", "-", "->", "|", "||",

	"&&",

	"@IDENT", "@STR", "@NUM",

	/* generated using :'<,'>s/TK_\(\w*\)/"\L\1"/g */
	"break", "continue", "else", "enum", "false", "fn", "for",
	"if", "impl", "let", "loop", "return", "self", "self_ty",
	"struct", "trait", "true", "type", "where", "while",

	"@eof", "@invalid"
};

/* generated using :'<,'>s/NT_\(\w*\)/"\L\1"/g */
const char *const nt_names[NT_COUNT] = {
	"decl",
	"fn_decl", "var_decl",
	"stmt",

	"expr_stmt",
	"for_stmt", "for_stmt_decl",
	"if_stmt", "if_optelse_stmt",
	"return_stmt",
	"while_stmt",
	"block", "block_decls",

	"expression", "optexpression",  /* expressions */
	"asgn", "asgn_opteq",  /* = */
	"logic_or", "logic_or_rest",  /* or */
	"logic_and", "logic_and_rest",  /* and */
	"equality", "equality_rest", "equality_op",  /* ==, != */
	"comparison", "comparison_rest", "comparison_op",  /* >, >=, <, <= */
	"term", "term_rest", "term_op",  /* +, - */
	"factor", "factor_rest", "factor_op",  /* *, / */
	"unary", "unary_op",
	"call", "call_optargs_or_getattr",
	"primary",

	"var_decl_optasgn",
	"fn",
	"fn_params", "fn_params_rest", "fn_optparams",
	"fn_args", "fn_args_rest", "fn_optargs",
};

const struct rdesc_grammar_symbol production_rules
	[NT_COUNT][NT_MAX_ALTERNATIVE_COUNT + 1][NT_MAX_ALTERNATIVE_SIZE + 1] = {
/* <decl> ::= */ r(
	NT(FN_DECL)
alt	NT(VAR_DECL)
alt	NT(STMT)
),

/* <fn-decl> ::= */ r(
	TK(FN), NT(FN)
),
/* <var-decl> ::= */ r(
	TK(LET), TK(IDENT), NT(VAR_DECL_OPTASGN), TK(SEMI)
),
/* <stmt> ::= */ r(
	NT(EXPR_STMT)
alt	NT(FOR_STMT)
alt	NT(IF_STMT)
alt	NT(RETURN_STMT)
alt	NT(WHILE_STMT)
alt	NT(BLOCK)
),

/* <expr-stmt> ::= */ r(
	NT(EXPRESSION), TK(SEMI)
),
/* <for-stmt> ::= */ r(
	TK(FOR), TK(LPAREN), NT(FOR_STMT_DECL),
		NT(OPTEXPRESSION), TK(SEMI),
		NT(OPTEXPRESSION), TK(RPAREN), NT(STMT)
),
/* <for-stmt-decl> ::= */ r(
	NT(VAR_DECL)
alt	NT(EXPR_STMT)
alt	TK(SEMI)
),
/* <if-stmt> ::= */ r(
	TK(IF), NT(EXPRESSION), NT(BLOCK), NT(IF_OPTELSE_STMT)
),
/* <if-optelse-stmt> ::= */ r(
	TK(ELSE), NT(BLOCK)
alt	EPSILON
),
/* <return-stmt> ::= */ r(
	TK(RETURN), NT(OPTEXPRESSION), TK(SEMI)
),
/* <while-stmt> ::= */ r(
	TK(WHILE), NT(EXPRESSION), NT(BLOCK)
),
/* <block> ::= */ r(
	TK(LBRACE), NT(BLOCK_DECLS), TK(RBRACE)
),
/* <block-decls> ::= */ r(
	NT(DECL), NT(BLOCK_DECLS)
alt	EPSILON
),

/* <expression> ::= */ r(
	NT(ASGN)
),
/* <optexpression> ::= */
	ropt(NT(EXPRESSION)),

/* <asgn> ::= */ r(
	NT(LOGIC_OR), NT(ASGN_OPTEQ)
),
/* <asgn-opteq> ::= */
	ropt(TK(EQ), NT(ASGN)),

/* <logic-or> ::= */
	rrr(LOGIC_OR, (NT(LOGIC_AND)), (TK(PIPE_PIPE), NT(LOGIC_AND))),
/* <logic-and> ::= */
	rrr(LOGIC_AND, (NT(EQUALITY)), (TK(AND_AND), NT(EQUALITY))),

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
alt	NT(CALL)
),
/* <unary-op> ::= */ r(
	TK(PLUS)
alt	TK(MINUS)
alt	TK(EXCL)
),

/* <call> ::= */ r(
	NT(PRIMARY), NT(CALL_OPTARGS_OR_GETATTR)
),
/* <call-optargs-or-get> ::= */ r(
	TK(LPAREN), NT(FN_OPTARGS), TK(RPAREN), NT(CALL_OPTARGS_OR_GETATTR)
alt	TK(DOT), TK(IDENT), NT(CALL_OPTARGS_OR_GETATTR)
alt	EPSILON
),

/* <primary> ::= */ r(
	TK(NUM)
alt	TK(STR)
alt	TK(TRUE)
alt	TK(FALSE)
alt	TK(LPAREN), NT(EXPRESSION), TK(RPAREN)
alt	TK(IDENT)
),

/* <var-decl-optasgn> ::= */
	ropt(TK(EQ), NT(EXPRESSION)),

/* <fn> ::= */ r(
	TK(IDENT), TK(LPAREN), NT(FN_OPTPARAMS), TK(RPAREN), NT(BLOCK)
),
/* <fn-params> ::= */
	rrr(FN_PARAMS, (TK(IDENT)), (TK(COMMA), TK(IDENT))),
/* <fn-optparams> ::= */
	ropt(NT(FN_PARAMS)),
/* <fn-args> ::= */
	rrr(FN_ARGS, (NT(EXPRESSION)), (TK(COMMA), NT(EXPRESSION))),
/* <fn-optargs> ::= */
	ropt(NT(FN_ARGS))
};
