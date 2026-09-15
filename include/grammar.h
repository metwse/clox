#ifndef GRAMMAR_H
#define GRAMMAR_H


enum token_type {
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
	TK_AND, TK_CLASS, TK_ELSE, TK_FALSE, TK_FUN, TK_IF, TK_NIL, TK_OR,
	TK_PRINT, TK_RETURN, TK_SUPER, TK_THIS, TK_TRUE, TK_VAR, TK_WHILE,

	TK_EOF, TK_INVALID
};

static const char *const punct[] = {
	"(", ")", "{", "}", ",", ".", "-", "+", ";", "/", "*",

	"!", "!=", "=", "==", ">", ">=", "<", "<=",
};

static const char *const keyword_names[] = {
	"and", "class", "else", "false", "fun", "if", "nil", "or",
	"print", "return", "super", "this", "true", "var", "while",
};

union seminfo {
	int ident_id;
	double num;
	char *str;
};

struct token {
	enum token_type ty;
	union seminfo seminfo;

	int line;
	int col;
};


#endif
