#include "../include/grammar.h"

#include "../vendor/rdesc/include/rule_macros.h"

#include "stdlib.h"


const char *const tk_names[TK_COUNT] = {
	"(", ")", "{", "}", ",", ".", "-", "+", ";", "/", "*",

	"!", "!=", "=", "==", ">", ">=", "<", "<=",

	"@IDENT", "@STR", "@NUM",

	"and", "class", "else", "false", "for", "fun", "if", "nil", "or",
	"print", "return", "super", "this", "true", "var", "while",

	"@eof", "@invalid"
};

const char *const nt_names[NT_COUNT] = {
	"decl",
	"class_decl", "fun_decl", "var_decl",
	"stmt",

	"expr_stmt",
	"for_stmt", "for_stmt_decl",
	"if_stmt", "if_optelse_stmt",
	"print_stmt",
	"return_stmt",
	"while_stmt",
	"block", "block_decls",

	"expression", "optexpression",
	"asgn", "asgn_opteq",
	"logic_or", "logic_or_rest",
	"logic_and", "logic_and_rest",
	"equality", "equality_rest", "equality_op",
	"comparison", "comparison_rest", "comparison_op",
	"term", "term_rest", "term_op",
	"factor", "factor_rest", "factor_op",
	"unary", "unary_op",
	"call", "call_optargs_or_getattr",
	"primary",

	"class_decl_optinheritance", "class_decl_functions",
	"var_decl_optasgn",
	"function",
	"function_params", "function_params_rest", "function_optparams",
	"function_args", "function_args_rest", "function_optargs",
};

const struct rdesc_grammar_symbol production_rules
	[NT_COUNT][NT_MAX_ALTERNATIVE_COUNT + 1][NT_MAX_ALTERNATIVE_SIZE + 1] = {
/* <decl> ::= */ r(
	NT(CLASS_DECL)
alt	NT(FUN_DECL)
alt	NT(VAR_DECL)
alt	NT(STMT)
),

/* <class-decl> ::= */ r(
	TK(CLASS), TK(IDENT), NT(CLASS_DECL_OPTINHERITANCE),
		TK(LBRACE), NT(CLASS_DECL_FUNCTIONS), TK(RBRACE)

),
/* <fun-decl> ::= */ r(
	TK(FUN), NT(FUNCTION)
),
/* <var-decl> ::= */ r(
	TK(VAR), TK(IDENT), NT(VAR_DECL_OPTASGN), TK(SEMI)
),
/* <stmt> ::= */ r(
	NT(EXPR_STMT)
alt	NT(FOR_STMT)
alt	NT(IF_STMT)
alt	NT(PRINT_STMT)
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
	TK(IF), TK(LPAREN), NT(EXPRESSION), TK(RPAREN), NT(STMT),
		NT(IF_OPTELSE_STMT)
),
/* <if-optelse-stmt> ::= */ r(
	TK(ELSE), NT(STMT)
alt	EPSILON
),
/* <print-stmt> ::= */ r(
	TK(PRINT), NT(EXPRESSION), TK(SEMI)
),
/* <return-stmt> ::= */ r(
	TK(RETURN), NT(OPTEXPRESSION), TK(SEMI)
),
/* <while-stmt> ::= */ r(
	TK(WHILE), TK(LPAREN), NT(EXPRESSION), TK(RPAREN), NT(STMT)
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
	rrr(LOGIC_OR, (NT(LOGIC_AND)), (TK(OR), NT(LOGIC_AND))),
/* <logic-and> ::= */
	rrr(LOGIC_AND, (NT(EQUALITY)), (TK(AND), NT(EQUALITY))),

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
	TK(LPAREN), NT(FUNCTION_OPTARGS), TK(RPAREN), NT(CALL_OPTARGS_OR_GETATTR)
alt	TK(DOT), TK(IDENT), NT(CALL_OPTARGS_OR_GETATTR)
alt	EPSILON
),

/* <primary> ::= */ r(
	TK(NUM)
alt	TK(STR)
alt	TK(TRUE)
alt	TK(FALSE)
alt	TK(NIL)
alt	TK(LPAREN), NT(EXPRESSION), TK(RPAREN)
alt	TK(THIS)
alt	TK(SUPER), TK(DOT), TK(IDENT)
alt	TK(IDENT)
),

/* <class-decl-optinheritance> ::= */
	ropt(TK(GT), TK(IDENT)),
/* <class-decl-functions> ::= */
	ropt(NT(FUNCTION), NT(CLASS_DECL_FUNCTIONS)),

/* <var-decl-optasgn> ::= */
	ropt(TK(EQ), NT(EXPRESSION)),

/* <function> ::= */ r(
	TK(IDENT), TK(LPAREN), NT(FUNCTION_OPTPARAMS), TK(RPAREN), NT(BLOCK)
),
/* <function-params> ::= */
	rrr(FUNCTION_PARAMS, (TK(IDENT)), (TK(COMMA), TK(IDENT))),
/* <function-optparams> ::= */
	ropt(NT(FUNCTION_PARAMS)),
/* <function-args> ::= */
	rrr(FUNCTION_ARGS, (NT(EXPRESSION)), (TK(COMMA), NT(EXPRESSION))),
/* <function-optargs> ::= */
	ropt(NT(FUNCTION_ARGS))
};
