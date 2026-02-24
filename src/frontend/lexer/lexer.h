#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Enumeration of all possible token types in the language.
 *
 */
typedef enum {
	// Keywords
	TK_STRUCT,
	TK_FN,
	TK_VOID,
	TK_RETURN,
	TK_WHILE,
	TK_FOR,
	TK_IF,
	TK_ELSE,
	TK_TRUE,
	TK_FALSE,
	TK_AS,	
	TK_CONST,
	TK_LET,

	//modules
	TK_EXPORT,
	TK_FROM,
	TK_IMPORT,

	// Data types
	// numeric
	TK_I8,
	TK_I16,
	TK_I32,
	TK_I64,
	TK_U8,
	TK_U16,
	TK_U32,
	TK_U64,

	TK_STRING,
	TK_FLOAT,
	TK_BOOL,
	TK_DOUBLE,

	// Literals
	TK_LIT,
	TK_STR,
	TK_NUM,

	// Arithmetic operators
	TK_PLUS,
	TK_MINUS,
	TK_STAR,
	TK_SLASH,
	TK_MOD,
	TK_INCR,
	TK_DECR,

	// Bitwise op
	// bitwise 'and' gonna be treated as TK_AMPERSAND for pointer purposes
	TK_BIT_OR,
	TK_BIT_XOR,
	TK_BIT_NOT,
	TK_LSHIFT,
	TK_RSHIFT,

	// Assignment operators
	TK_ASSIGN,
	TK_PLUS_ASSIGN,
	TK_MINUS_ASSIGN,
	TK_STAR_ASSIGN,
	TK_SLASH_ASSIGN,
	TK_AND_ASSIGN,
	TK_OR_ASSIGN,
	TK_XOR_ASSIGN,
	TK_NOT_ASSIGN,
	TK_LSHIFT_ASSIGN,
	TK_RSHIFT_ASSIGN,

	// Comparison operators
	TK_EQ,
	TK_NOT_EQ,
	TK_LESS,
	TK_GREATER,
	TK_LESS_EQ,
	TK_GREATER_EQ,

	// Logical operators
	TK_AND,
	TK_OR,
	TK_NOT,

	// Delimiters
	TK_LBRACE,
	TK_RBRACE,
	TK_LPAREN,
	TK_RPAREN,
	TK_SEMI,
	TK_COMMA,
	TK_QUOTE,
	TK_ARROW,
	TK_QUESTION,
	TK_COLON,
	TK_DOT,
	TK_AMPERSAND,
	TK_LBRACKET,
	TK_RBRACKET,

	// Special tokens
	TK_NULL,
	TK_EOF,
	TK_INVALID
} TokenType;

typedef struct Token {
    TokenType type;
    const char *start;
	uint16_t length;
    uint16_t line;
    uint16_t column;
} Token;

typedef struct TokenList {
	Token *tokens;
	size_t count;
	size_t capacity;
	char * buffer;
	char *filename;
}TokenList;

typedef struct {
	const char *src;
	const char *cur;
	size_t line;
	size_t col;
	size_t line_start;
	TokenList *list;
} Lexer;

TokenList* lex(const char *input, const char *filename);
void freeTokens(TokenList *list);
const char* tokenName(TokenType type);
char *extractSourceLineForToken(TokenList *list, Token *token);

#endif //LEXER_H
