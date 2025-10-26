#include <stdio.h>
#include <stdlib.h>

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

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,
    PREC_OR, // or
    PREC_AND, // and
    PREC_BITOR, // |
    PREC_BITXOR, // ^
    PREC_BITAND, // &
    PREC_EQUALITY, // == ===
    PREC_COMPARISON, // > < >= <=
    PREC_SHIFT, // >> <<
    PREC_TERM, // + -
    PREC_FACTOR, // * / %
    PREC_EXPONENTIAL, // **
    PREC_UNARY, // ! ~ - #
    PREC_CALL, // . () []
    PREC_PRIMARY,
} Precedence;

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

static uint8_t makeConstant(Value value) {
    int constant = addConstant(parser.thisChunk, value);
    // TODO handle long_constant
    return (uint8_t)constant;
}

static void emitConstant(Value value) {
    emitBytes(OP_CONSTANT, makeConstant(value));
}

static void endCompiler() {
    emitReturn();
}

static void expression() {
    // Vaughan Pratt's top-down operator precedence parsing
    // Parse an expression starting at the lowest precedence levels
    parsePrecedence(PREC_ASSIGNMENT);
}

static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect end ')' after expression.");
}

static void unary() {
    TokenType opType = parser.previous.type;
    parsePrecedence(PREC_UNARY);
    switch (opType) {
        case TOKEN_MINUS:
            emitByte(OP_NEG);
            break;
        default:
            return;
    }
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(value);
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
