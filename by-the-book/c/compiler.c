#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "scanner.h"
#include "vm.h"
#include "chunk.h"
#include "debug.h"

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

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

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
        if (parser.current.type != TOKEN_ERROR) {
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

static void endCompiler(bool debugPrint) {
    emitReturn();
    if (debugPrint) {
        disChunk(currentChunk(), "code");
    }
}

static void expression();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence prec);

static void binary() {
    TokenType opType = parser.previous.type;
    ParseRule* rule = getRule(opType);
    parsePrecedence((Precedence)(rule->precedence + 1));

    // TODO exhaustive
    switch (opType) {
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUB); break;
        case TOKEN_STAR: emitByte(OP_MUL); break;
        case TOKEN_SLASH: emitByte(OP_DIV); break;
        case TOKEN_STAR_STAR: emitByte(OP_EXP); break;
        case TOKEN_REMAINDER: emitByte(OP_REMAINDER); break;
        case TOKEN_BITAND: emitByte(OP_BITAND); break;
        case TOKEN_BITOR: emitByte(OP_BITOR); break;
        case TOKEN_BITXOR: emitByte(OP_BITXOR); break;
        case TOKEN_BITNEG: emitByte(OP_BITNEG); break;
        case TOKEN_LEFT_SHIFT: emitByte(OP_LEFT_SHIFT); break;
        case TOKEN_RIGHT_SHIFT: emitByte(OP_RIGHT_SHIFT); break;
        case TOKEN_LEFT_PAREN:
        case TOKEN_RIGHT_PAREN:
        case TOKEN_LEFT_BRACE:
        case TOKEN_RIGHT_BRACE:
        case TOKEN_RIGHT_SQUARE_BRACE:
        case TOKEN_LEFT_SQUARE_BRACE:
        case TOKEN_COMMA:
        case TOKEN_DOT:
        case TOKEN_SEMICOLON:
        case TOKEN_SIZE:
        case TOKEN_BANG:
        case TOKEN_EQUAL:
        case TOKEN_GREAT:
        case TOKEN_LESS: // TODO <
        case TOKEN_BANG_EQUAL:
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_GREAT_EQUAL:
        case TOKEN_LESS_EQUAL:
        case TOKEN_IDENTIFIER:
        case TOKEN_STRING:
        case TOKEN_NUMBER:
        case TOKEN_INTEGER:
        case TOKEN_HEXINT:
        case TOKEN_AND:
        case TOKEN_CLASS:
        case TOKEN_ELSE:
        case TOKEN_FALSE:
        case TOKEN_FOR:
        case TOKEN_FUN:
        case TOKEN_IF:
        case TOKEN_NIL:
        case TOKEN_OR:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
        case TOKEN_SUPER:
        case TOKEN_THIS:
        case TOKEN_TRUE:
        case TOKEN_VAR:
        case TOKEN_WHILE:
        case TOKEN_ERROR:
        case TOKEN_EOF:
            break;
    }
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
        case TOKEN_MINUS: emitByte(OP_NEG); break;
        case TOKEN_PLUS: break;
        case TOKEN_SIZE: emitByte(OP_SIZE); break;
        case TOKEN_BITNEG: emitByte(OP_BITNEG); break;
        case TOKEN_LEFT_PAREN:
        case TOKEN_RIGHT_PAREN:
        case TOKEN_LEFT_BRACE:
        case TOKEN_RIGHT_BRACE:
        case TOKEN_RIGHT_SQUARE_BRACE: // TODO new token, lists
        case TOKEN_LEFT_SQUARE_BRACE:
        case TOKEN_COMMA:
        case TOKEN_DOT:
        case TOKEN_SEMICOLON:
        case TOKEN_BITAND: // TODO bit-ops
        case TOKEN_BITOR: // TODO bit-ops
        case TOKEN_BITXOR: // TODO bit-ops
        case TOKEN_BANG:
        case TOKEN_EQUAL:
        case TOKEN_GREAT:
        case TOKEN_SLASH:
        case TOKEN_REMAINDER: // TODO %
        case TOKEN_STAR:
        case TOKEN_STAR_STAR: // TODO exponential
        case TOKEN_LESS: // TODO <
    // Two or more characters
        case TOKEN_BANG_EQUAL:
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_GREAT_EQUAL:
        case TOKEN_LESS_EQUAL:
        case TOKEN_LEFT_SHIFT: // TODO bit-ops
        case TOKEN_RIGHT_SHIFT: // TODO bit-ops
    // Literals
        case TOKEN_IDENTIFIER:
        case TOKEN_STRING:
        case TOKEN_NUMBER:
        case TOKEN_INTEGER:
        case TOKEN_HEXINT:
    // Keywords
        case TOKEN_AND:
        case TOKEN_CLASS:
        case TOKEN_ELSE:
        case TOKEN_FALSE:
        case TOKEN_FOR:
        case TOKEN_FUN:
        case TOKEN_IF:
        case TOKEN_NIL:
        case TOKEN_OR:
        case TOKEN_PRINT:
        case TOKEN_RETURN:
        case TOKEN_SUPER:
        case TOKEN_THIS:
        case TOKEN_TRUE:
        case TOKEN_VAR:
        case TOKEN_WHILE:
        case TOKEN_ERROR:
    // Special
        case TOKEN_EOF:
            break;
    }
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(DOUBLE_VAL(value));
}

static void integer() {
    int value = strtol(parser.previous.start, NULL, 10);
    emitConstant(INTEGER_VAL(value));
}

static void hexnumber() {
    int value = strtol(parser.previous.start, NULL, 16);
    emitConstant(INTEGER_VAL(value));
}

ParseRule rules[] = {
    // TODO exhaustive
    [TOKEN_LEFT_PAREN]         = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN]        = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE]         = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE]        = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_SQUARE_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_SQUARE_BRACE]  = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA]              = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT]                = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS]              = {unary, binary, PREC_TERM},
    [TOKEN_PLUS]               = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON]          = {NULL, NULL, PREC_NONE},
    [TOKEN_SIZE]               = {unary, NULL, PREC_NONE},
    [TOKEN_BITAND]             = {NULL, binary, PREC_BITAND},
    [TOKEN_BITOR]              = {NULL, binary, PREC_BITOR},
    [TOKEN_BITXOR]             = {NULL, binary, PREC_BITXOR},
    [TOKEN_BITNEG]             = {unary, NULL, PREC_NONE},
    [TOKEN_BANG]               = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL]              = {NULL, NULL, PREC_NONE},
    [TOKEN_GREAT]              = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH]              = {NULL, binary, PREC_FACTOR},
    [TOKEN_REMAINDER]          = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR]               = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR_STAR]          = {NULL, binary, PREC_EXPONENTIAL},
    [TOKEN_LESS]               = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL]         = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL]        = {NULL, NULL, PREC_NONE},
    [TOKEN_GREAT_EQUAL]        = {NULL, NULL, PREC_NONE},
    [TOKEN_LESS_EQUAL]         = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_SHIFT]         = {NULL, binary, PREC_SHIFT},
    [TOKEN_RIGHT_SHIFT]        = {NULL, binary, PREC_SHIFT},
    [TOKEN_IDENTIFIER]         = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING]             = {NULL, NULL, PREC_NONE},
    [TOKEN_NUMBER]             = {number, NULL, PREC_NONE},
    [TOKEN_INTEGER]            = {integer, NULL, PREC_NONE},
    [TOKEN_HEXINT]             = {hexnumber, NULL, PREC_NONE},
    [TOKEN_AND]                = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS]              = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE]               = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE]              = {NULL, NULL, PREC_NONE},
    [TOKEN_FOR]                = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN]                = {NULL, NULL, PREC_NONE},
    [TOKEN_IF]                 = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL]                = {NULL, NULL, PREC_NONE},
    [TOKEN_OR]                 = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT]              = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN]             = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER]              = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS]               = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE]               = {NULL, NULL, PREC_NONE},
    [TOKEN_VAR]                = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE]              = {NULL, NULL, PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void parsePrecedence(Precedence precedence) {
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if (prefixRule == NULL) {
        error("Expect expression.");
        return;
    }
    // The first token is ALWAYS a prefix, these include values and parenthesis groupings
    prefixRule();

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

bool compile(const char* source, Chunk* chunk, bool debugPrint) {
    parser.hadError = false;
    parser.panicMode = false;
    parser.thisChunk = chunk;
    initScanner(source);
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler(debugPrint);
    return !parser.hadError;
}
