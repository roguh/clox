#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "scanner.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"

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

typedef void (*ParseFn)(bool canAssign);

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

typedef struct {
    Token name;
    int depth;
} Local;

typedef struct {
    int localCount;
    int scopeDepth;
    int localsSize;
    Local locals[];
} Compiler;

Parser parser;
Compiler* current = NULL;

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

static Chunk* currentChunk(void) {
    return parser.thisChunk;
}

static void advance(void) {
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

static bool check(TokenType type) {
    return parser.current.type == type;
}

static bool match(TokenType type) {
    if (!check(type)) {
        return false;
    }
    advance();
    return true;
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line, parser.previous.column);
}

static void emitReturn(void) {
    emitByte(OP_RETURN);
}

static void emitBytes(uint8_t b1, uint8_t b2) {
    emitByte(b1);
    emitByte(b2);
}

static int makeConstant(Value value) {
    int constant = addConstant(parser.thisChunk, value);
    return constant;
}

static int emitConstant(Value value) {
    return writeConstant(currentChunk(), value, parser.previous.line, parser.previous.column);
}

static void initCompiler(size_t size) {
    Compiler* compiler = malloc(sizeof(Compiler) + sizeof(Local) * size);
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->localsSize = size;
    current = compiler;
}

static void defineVariable(int global) {
    if (current->scopeDepth == 0) {
        writeConstantByOffset(currentChunk(), OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_LONG, global, parser.previous.line, parser.previous.column);
    }
    // No code needed to define local variables at runtime
}

static void endCompiler(bool debugPrint) {
    emitReturn();
    if (debugPrint) {
        disChunk(currentChunk(), "code");
    }
}

static void beginScope() {
    current->scopeDepth++;
}

static void endScope() {
    current->scopeDepth--;
    while (current->localCount > 0
        && current->locals[current->localCount - 1].depth > current->scopeDepth) {
        emitByte(OP_POP);
        current->localCount--;
    }
}

static void expression(void);
static void statement(void);
static void declaration(void);
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence prec);

static void binary(bool canAssign) {
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
        case TOKEN_EQUAL_EQUAL: emitByte(OP_EQUAL); break;
        case TOKEN_GREAT: emitByte(OP_GREATER); break;
        case TOKEN_LESS: emitByte(OP_LESS); break;
        case TOKEN_BANG_EQUAL: emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_GREAT_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS_EQUAL: emitBytes(OP_GREATER, OP_NOT); break;

        case TOKEN_EQUAL:

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

static void expression(void) {
    // Vaughan Pratt's top-down operator precedence parsing
    // Parse an expression starting at the lowest precedence levels
    parsePrecedence(PREC_ASSIGNMENT);
}

static void block() {
    while (!(check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static int identifierConstant(Token* name) {
    return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
}

static bool identifiersEqual(Token* a, Token* b) {
    if (a->length != b->length) {
        return false;
    }
    return memcmp(a->start, b->start, a->length) == 0;
}

static void addLocal(Token name) {
    Local* local = &current->locals[current->localCount++];
    local->name = name;
    local->depth = current->scopeDepth;
    // TODO realloc if too many locals
    if (current->localCount == 1024) {
        error("Too many local variables.");
        return ;
    }
}

static void declareVariable() {
    if (current->scopeDepth == 0) {
        return;
    }
    Token name = parser.previous;
    for (int i = current->localCount - 1; i >= 0; i--) {
        Local local = current->locals[i];
        // Iterate through all variables in the current scope
        // Backwards for-loop, check name equality
        if (local.depth != -1 && local.depth < current->scopeDepth) {
            break;
        }
        if (identifiersEqual(&name, &local.name)) {
            error("A variable exists with this name in this scope.");
        }
    }
    addLocal(name);
}

static int parseVariable(const char* errMessage) {
    consume(TOKEN_IDENTIFIER, errMessage);
    declareVariable();
    if (current->scopeDepth > 0) {
        return 0;
    }
    // A name-lookup is only needed for globals
    return identifierConstant(&parser.previous);
}

static void varDeclaration(void) {
    int global = parseVariable("Expect variable name.");
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    defineVariable(global);
}

static void printStatement(void) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after print.");
    emitByte(OP_PRINT);
}

static void expressionStatement(void) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after print.");
    emitByte(OP_POP);
}

static void statement(void) {
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
}

static void synchronize(void) {
    parser.panicMode = false;
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) {
            return;
        }
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;
            default:
                ;
        }
    }
    advance();
}

static void declaration(void) {
    if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) {
        synchronize();
    }
}

static void grouping(bool canAssign) {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect end ')' after expression.");
}

static void unary(bool canAssign) {
    TokenType opType = parser.previous.type;
    parsePrecedence(PREC_UNARY);
    switch (opType) {
        case TOKEN_MINUS: emitByte(OP_NEG); break;
        case TOKEN_PLUS: break;
        case TOKEN_SIZE: emitByte(OP_SIZE); break;
        case TOKEN_BITNEG: emitByte(OP_BITNEG); break;
        case TOKEN_BANG: emitByte(OP_NOT); break;

        case TOKEN_PRINT:
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

static void literal(bool canAssign) {
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        default:
            return;
    }
}

static void string(bool canAssign) {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
}

static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local local = compiler->locals[i];
        if (identifiersEqual(name, &local.name)) {
            return i;
        }
    }
    return -1;
}

static void namedVariable(Token name, bool canAssign) {
    int offset = resolveLocal(current, &name);

    OpCode instr, instrLong;
    if (canAssign && match(TOKEN_EQUAL)) {
        expression();
        if (offset != -1) {
            // Local variables
            instr = OP_SET_LOCAL;
            instrLong = OP_SET_LOCAL_LONG;
        } else {
            // Global variables
            offset = identifierConstant(&name);
            instr = OP_SET_GLOBAL;
            instrLong = OP_SET_GLOBAL_LONG;
        }
    } else {
        if (offset != -1) {
            // Local variables
            instr = OP_GET_LOCAL;
            instrLong = OP_GET_LOCAL_LONG;
        } else {
            // Global variables
            offset = identifierConstant(&name);
            instr = OP_GET_GLOBAL;
            instrLong = OP_GET_GLOBAL_LONG;
        }
    }
    writeConstantByOffset(currentChunk(), instr, instrLong, offset, parser.previous.line, parser.previous.column);
}

static void variable(bool canAssign) {
    namedVariable(parser.previous, canAssign);
}

static void number(bool canAssign) {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(DOUBLE_VAL(value));
}

static void integer(bool canAssign) {
    int value = strtol(parser.previous.start, NULL, 10);
    emitConstant(INTEGER_VAL(value));
}

static void hexnumber(bool canAssign) {
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
    [TOKEN_BANG]               = {unary, NULL, PREC_NONE},
    [TOKEN_SLASH]              = {NULL, binary, PREC_FACTOR},
    [TOKEN_REMAINDER]          = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR]               = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR_STAR]          = {NULL, binary, PREC_EXPONENTIAL},
    [TOKEN_EQUAL]              = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREAT]              = {NULL, binary, PREC_EQUALITY},
    [TOKEN_LESS]               = {NULL, binary, PREC_EQUALITY},
    [TOKEN_BANG_EQUAL]         = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL_EQUAL]        = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREAT_EQUAL]        = {NULL, binary, PREC_EQUALITY},
    [TOKEN_LESS_EQUAL]         = {NULL, binary, PREC_EQUALITY},
    [TOKEN_LEFT_SHIFT]         = {NULL, binary, PREC_SHIFT},
    [TOKEN_RIGHT_SHIFT]        = {NULL, binary, PREC_SHIFT},
    [TOKEN_IDENTIFIER]         = {variable, NULL, PREC_NONE},
    [TOKEN_STRING]             = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER]             = {number, NULL, PREC_NONE},
    [TOKEN_INTEGER]            = {integer, NULL, PREC_NONE},
    [TOKEN_HEXINT]             = {hexnumber, NULL, PREC_NONE},
    [TOKEN_AND]                = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS]              = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE]               = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE]              = {literal, NULL, PREC_NONE},
    [TOKEN_FOR]                = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN]                = {NULL, NULL, PREC_NONE},
    [TOKEN_IF]                 = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL]                = {literal, NULL, PREC_NONE},
    [TOKEN_OR]                 = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT]              = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN]             = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER]              = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS]               = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE]               = {literal, NULL, PREC_NONE},
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
    bool canAssign = precedence <= PREC_ASSIGNMENT;
    prefixRule(canAssign);

    while (precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule(canAssign);
    }

    if (canAssign && match(TOKEN_EQUAL)) {
        error("Invalid assignment target.");
    }
}

bool compile(const char* source, Chunk* chunk, bool debugPrint) {
    parser.hadError = false;
    parser.panicMode = false;
    parser.thisChunk = chunk;
    initScanner(source);
    initCompiler(1024);
    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_EOF, "Expect end of expression.");
    endCompiler(debugPrint);
    return !parser.hadError;
}
