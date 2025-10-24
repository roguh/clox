#ifndef clox_scanner_h
#define clox_scanner_h

typedef enum {
    // Single-character tokens
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
    TOKEN_SLASH,
    TOKEN_SIZE, // Lua's size operator
    TOKEN_STAR,
    TOKEN_BANG,
    TOKEN_EQUAL,
    TOKEN_GREAT,
    TOKEN_LESS, // TODO <
    TOKEN_BITAND, // TODO bit-ops
    TOKEN_BITOR, // TODO bit-ops
    TOKEN_BITNEG, // TODO bit-ops
    TOKEN_BITXOR, // TODO bit-ops
    // Two or more characters
    TOKEN_BANG_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_GREAT_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_STAR_STAR, // TODO exponential
    TOKEN_LEFT_SHIFT, // TODO bit-ops
    TOKEN_RIGHT_SHIFT, // TODO bit-ops
    TOKEN_EQUAL_EQUAL_EQUAL, // TODO new token
    TOKEN_EQUAL_EQUAL_EQUAL_EQUAL, // TODO new token
    // Literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    // Keywords
    TOKEN_AND,
    TOKEN_CLASS,
    TOKEN_FALSE,
    TOKEN_FOR,
    TOKEN_FUN,
    TOKEN_IF,
    TOKEN_NIL,
    TOKEN_OR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_SUPER,
    TOKEN_THIS,
    TOKEN_TRUE,
    TOKEN_VAR,
    TOKEN_WHILE,
    TOKEN_ERROR,
    // Special
    TOKEN_EOF,
} TokenType;

typedef struct {
    TokenType type;
    const char* start;
    int length;
    int line;
    int column;
} Token;

Token scanToken();
void initScanner(const char* source);
void scanAndPrint(const char* source);

#endif
