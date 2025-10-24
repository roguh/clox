#include <stdio.h>
#include "common.h"
#include "scanner.h"

typedef struct {
    const char* start;
    const char* current;
    int line;
    int column;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
    scanner.column = 0;
}

static bool isAtEnd() {
    return *scanner.current == '\0';
}

static Token makeToken(TokenType ty) {
    Token tk;
    tk.type = ty;
    tk.start = scanner.start;
    tk.length = (int)(scanner.current - scanner.start);
    tk.line = scanner.line;
    tk.column = scanner.column;
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

static char advance() {
    scanner.current++;
    scanner.column++;
    return scanner.current[-1];
}

static bool match(char expected) {
    if (isAtEnd()) {
        return false;
    }
    if (*scanner.current != expected) {
        return false;
    }
    scanner.current++;
    return true;
}

static char peek() {
    return *scanner.current;
}

static char peekNext() {
    if (isAtEnd()) {
        return '\0';
    }
    return *(scanner.current + 1);
}

static void skipWhitespace() {
    while (true) {
        char c = peek();
        switch (c) {
            case '\n':
                scanner.line++;
                scanner.column = 0;
                // Fallthrough
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

Token scanToken() {
    skipWhitespace();
    scanner.start = scanner.current;
    if (isAtEnd()) {
        return makeToken(TOKEN_EOF);
    }
    char c = advance();
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
        case '~': return makeToken(TOKEN_BITNEG);
        case '^': return makeToken(TOKEN_BITXOR);
        case '!': return makeToken(TOKEN_BANG);
        // Division or single-line comment or multi-line comment
        case '/': return makeToken(TOKEN_SLASH);
        // One or Two-character tokens
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
    }
    return errorToken("Unexpected character");
}

void scanAndPrint(const char* source) {
    initScanner(source);
    while (true) {
        Token tk = scanToken();
        printf("%4d:%-4d ", tk.line, tk.column);
        printf("'%.*s'\n", tk.length, tk.start);

        if (tk.type == TOKEN_EOF) {
            break;
        }
    }
}
