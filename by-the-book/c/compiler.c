#include <stdio.h>

#include "common.h"
#include "scanner.h"
#include "vm.h"

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Chunk* thisChunk;
} Parser;

Parser parser;

static void errorAt(Token* token, const char* message) {
    if (parser.panicMode) {
        return;
    }
    parser.panicMode = true;
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

static Chunk* currentChunk() {
    return parser.thisChunk;
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line, parser.previous.column);
}

static void emitReturn() {
    emitByte(OP_RETURN);
}

static void emitBytes(uint8_t b1, uint8_t b2) {
    emitByte(b1);
    emitByte(b2);
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

static void consume(TokenType type, const char* message) {
    if (parser.current.type == type) {
        advance();
        return;
    }
    errorAtCurrent(message);
}

static void endCompiler() {
    emitReturn();
}

bool compile(const char* source, Chunk* chunk) {
    parser.hadError = false;
    parser.panicMode = false;
    parser.thisChunk = chunk;
    initScanner(source);
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler();
    return !parser.hadError;
}
