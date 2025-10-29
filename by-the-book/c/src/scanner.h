#ifndef clox_scanner_h
#define clox_scanner_h

#include "common.h"

typedef enum {
    // Single-character tokens
    TOKEN_COLON,
    TOKEN_LEFT_PAREN,
    TOKEN_RIGHT_PAREN,
    TOKEN_LEFT_BRACE,
    TOKEN_RIGHT_BRACE,
    TOKEN_RIGHT_SQUARE_BRACE, // TODO new token, lists
    TOKEN_LEFT_SQUARE_BRACE,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_MINUS,
    TOKEN_PLUS,
    TOKEN_SEMICOLON,
    TOKEN_SIZE, // Lua's size operator
    TOKEN_BITAND, // TODO bit-ops
    TOKEN_BITOR, // TODO bit-ops
    TOKEN_BITXOR, // TODO bit-ops
    TOKEN_BITNEG, // TODO bit-ops
    TOKEN_BANG,
    TOKEN_EQUAL,
    TOKEN_GREAT,
    TOKEN_SLASH,
    TOKEN_REMAINDER, // TODO %
    TOKEN_STAR,
    TOKEN_STAR_STAR, // TODO exponential
    TOKEN_LESS, // TODO <
    // Two or more characters
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREAT_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_LEFT_SHIFT, // TODO bit-ops
    TOKEN_RIGHT_SHIFT, // TODO bit-ops
    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_INTEGER,
    TOKEN_HEXINT,
    // Keyword literals
    TOKEN_FALSE,
    TOKEN_INF,
    TOKEN_NAN,
    TOKEN_NIL,
    TOKEN_TRUE,
    // Keywords
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_VAR,
    TOKEN_WHILE,
    TOKEN_ERROR,
    // Special
    TOKEN_EOF,
} TokenType;

static bool isNumber(TokenType ty) {
    return ty == TOKEN_NUMBER || ty == TOKEN_HEXINT || ty == TOKEN_INTEGER;
}

static bool isKeyword(TokenType ty) {
    return ty == TOKEN_AND || ty == TOKEN_CLASS || ty == TOKEN_ELSE || ty == TOKEN_FALSE || ty == TOKEN_FOR || ty == TOKEN_FUN || ty == TOKEN_IF || ty == TOKEN_NIL || ty == TOKEN_OR || ty == TOKEN_PRINT || ty == TOKEN_RETURN || ty == TOKEN_SUPER || ty == TOKEN_THIS || ty == TOKEN_TRUE || ty == TOKEN_VAR || ty == TOKEN_WHILE || ty == TOKEN_ERROR;
}

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
    int column;
    int startLine;
    int startColumn;
} Token;

Token scanToken(void);
void initScanner(const char* source);
void scanAndPrint(const char* source);

#endif
