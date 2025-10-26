#include <stdio.h>

#include "common.h"
#include "scanner.h"
#include "vm.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
} Parser;

Parser parser;

static void errorAt(const char* message) {
    ERR_PRINT("[%d:%d] Error", token->line, token->column);
    if (token->type == TOKEN_EOF) {
        ERR_PRINT(" at end of input");
    } else if (token->type == TOKEN_ERROR) {
    } else {
        ERR_PRINT(" at %.*s", token->length, token->start);
    }
    ERR_PRINT(": %s\n", message);
    parser.hadError = true;
}

static void error(const char* message) {
    errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
    errorAt(&parser.current, message);
}

static void advance() {
    parser.previous = parser.current;
    while (true) {
        parser.current = scanToken();
        if (parser.current.type  != TOKEN_ERROR) {
            break;
        }
        errorAtCurrent(parser.current.start);
    }
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    return true;
}
