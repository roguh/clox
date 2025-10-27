#include <stdio.h>
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
    int startLine;
    int startColumn;
} Scanner;

typedef enum {
    STR_DOUBLE,
    STR_SINGLE,
} StringType;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.column = 0;
    scanner.startLine = scanner.line;
    scanner.startColumn = scanner.column;
}

static bool isAtEnd(void) {
    return *scanner.current == '\0';
}

static Token makeToken(TokenType ty) {
    Token tk;
    tk.type = ty;
    tk.start = scanner.start;
    tk.length = (int)(scanner.current - scanner.start);
    tk.line = scanner.line;
    tk.column = scanner.column;
    tk.startLine = scanner.startLine;
    tk.startColumn = scanner.startColumn;
    return tk;
}

static Token errorToken(const char* message) {
    Token tk;
    tk.type = TOKEN_ERROR;
    tk.start = message;
    tk.length = (int)strlen(message);
    tk.line = scanner.line;
    tk.column = scanner.column;
    return tk;
}

static char advance(void) {
    scanner.current++;
    scanner.column++;
    char c = scanner.current[-1];
    if (c == '\n') {
        scanner.line++;
        scanner.column = 0;
    }
    return c;
}

static char peek(void) {
    return *scanner.current;
}

static bool match(char expected) {
    if (isAtEnd()) {
        return false;
    }
    if (peek() != expected) {
        return false;
    }
    advance();
    return true;
}

static char peekNext(void) {
    if (isAtEnd()) {
        return '\0';
    }
    return *(scanner.current + 1);
}

static bool isDigit(char c, bool hex) {
    if (hex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
        return true;
    }
    return c >= '0' && c <= '9';
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static TokenType checkKeyword(int start, int length, const char* rest, TokenType type) {
    if (scanner.current - scanner.start == start + length &&
        memcmp(scanner.start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifierType(void) {
    // Check the start of the token to see if it's a keyword
    switch (scanner.start[0]) {
    case 'a': return checkKeyword(1, 2, "nd", TOKEN_AND);
    case 'c': return checkKeyword(1, 4, "lass", TOKEN_CLASS);
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'n': return checkKeyword(1, 2, "il", TOKEN_NIL);
    case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'p': return checkKeyword(1, 4, "rint", TOKEN_PRINT);
    case 'r': return checkKeyword(1, 5, "eturn", TOKEN_RETURN);
    case 's': return checkKeyword(1, 4, "uper", TOKEN_SUPER);
    case 'v': return checkKeyword(1, 2, "ar", TOKEN_VAR);
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);

    // Keywords starting with f: false for fun
    case 'f':
        // A token of length > 1
        if (scanner.current - scanner.start > 1) {
            switch(scanner.start[1]) {
                case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
                case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
                case 'u': return checkKeyword(2, 1, "n", TOKEN_FUN);
            }
        }
        break;

    // Keywords starting with t: this true
    case 't':
        // A token of length > 1
        if (scanner.current - scanner.start > 1) {
            switch(scanner.start[1]) {
                case 'h': return checkKeyword(2, 2, "is", TOKEN_THIS);
                case 'r': return checkKeyword(2, 2, "ue", TOKEN_TRUE);
            }
        }
        break;
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier(void) {
    while (isAlpha(peek()) || isDigit(peek(), false)) {
        advance();
    }
    return makeToken(identifierType());
}

static Token hexnum(void) {
    // Starting 0x
    advance();
    while (isDigit(peek(), true)) {
        advance();
    }
    return makeToken(TOKEN_HEXINT);
}

static Token number(void) {
    bool hasDigit = false;
    while (isDigit(peek(), false)) {
        advance();
    }
    if (peek() == '.' && isDigit(peekNext(), false)) {
        hasDigit = true;
        advance();
        while (isDigit(peek(), false)) {
            advance();
        }
    }
    if (hasDigit) {
        return makeToken(TOKEN_NUMBER);
    } else {
        return makeToken(TOKEN_INTEGER);
    }
}

static Token string(StringType ty) {
    char end = ty == STR_DOUBLE ? '"' : '\'';
    while (peek() != end && !isAtEnd()) {
        if (peek() == '\\') {
            advance();
        }
        advance();
    }

    if (isAtEnd()) {
        return errorToken("Unterminated string.");
    }
    // Consume closing quote
    advance();
    return makeToken(TOKEN_STRING);
}

static void skipWhitespace(void) {
    while (true) {
        char c = peek();
        switch (c) {
            case '\n':
            case ' ':
            case '\t':
            case '\r':
                advance();
                break;
            case '/':
                if (peekNext() == '*') {
                    while (!((peek() == '*' && peekNext() == '/') || isAtEnd())) {
                        advance();
                    }
                    if (!isAtEnd()) {
                        advance();
                        advance();
                    }
                } else if (peekNext() == '/') {
                    while (!(peek() == '\n' || isAtEnd())) {
                        advance();
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

Token scanToken(void) {
    skipWhitespace();
    scanner.startLine = scanner.line;
    scanner.startColumn = scanner.column;
    scanner.start = scanner.current;
    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }
    char c = advance();
    if (isAlpha(c)) {
        return identifier();
    }
    if (c == '0' && (peek() == 'X' || peek() == 'x')) {
        return hexnum();
    }
    if ((c == '.' && isDigit(peek(), false)) || isDigit(c, false)) {
        return number();
    }
    switch (c) {
        // One character tokens
        case '(': return makeToken(TOKEN_LEFT_PAREN);
        case ')': return makeToken(TOKEN_RIGHT_PAREN);
        case '{': return makeToken(TOKEN_LEFT_BRACE);
        case '}': return makeToken(TOKEN_RIGHT_BRACE);
        case '[': return makeToken(TOKEN_RIGHT_SQUARE_BRACE);
        case ']': return makeToken(TOKEN_LEFT_SQUARE_BRACE);
        case ',': return makeToken(TOKEN_COMMA);
        case '.': return makeToken(TOKEN_DOT);
        case '-': return makeToken(TOKEN_MINUS);
        case '+': return makeToken(TOKEN_PLUS);
        case ';': return makeToken(TOKEN_SEMICOLON);
        case '#': return makeToken(TOKEN_SIZE);
        case '&': return makeToken(TOKEN_BITAND);
        case '|': return makeToken(TOKEN_BITOR);
        case '^': return makeToken(TOKEN_BITXOR);
        case '~': return makeToken(TOKEN_BITNEG);
        case '/': return makeToken(TOKEN_SLASH);
        case '%': return makeToken(TOKEN_REMAINDER);
        // One or Two-character tokens
        case '!': return makeToken(match('=') ?
            TOKEN_BANG_EQUAL : TOKEN_BANG);
        case '*': return makeToken(match('*') ?
            TOKEN_STAR_STAR : TOKEN_STAR);
        case '=': return makeToken(match('=') ?
            TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
        case '>': return makeToken(match('=') ?
            TOKEN_GREAT_EQUAL :
                (match('>') ? TOKEN_RIGHT_SHIFT: TOKEN_GREAT));
        case '<': return makeToken(match('=') ?
            TOKEN_LESS_EQUAL :
                (match('<') ? TOKEN_LEFT_SHIFT : TOKEN_LESS));
        case '"':
            return string(STR_DOUBLE);
        case '\'':
            return string(STR_SINGLE);
    }
    return errorToken("Unexpected character");
}

void scanAndPrint(const char* source) {
    initScanner(source);
    while (true) {
        Token tk = scanToken();
        printf("%4d:%-4d ", tk.line, tk.column);
        if (tk.type == TOKEN_EOF) {
            printf("EOF\n");
            break;
        } else if (isKeyword(tk.type) || isNumber(tk.type)) {
            printf("%.*s\n", tk.length, tk.start);
        } else {
            printf("'%.*s'\n", tk.length, tk.start);
        }
    }
}
