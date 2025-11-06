#include <stdio.h>
#include <stdlib.h>

#include "scanner.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"

bool DEBUG_PARSER = false;
static int depth = 4;
#define debugp(name) { if (DEBUG_PARSER) { fprintf(stderr, "%*s%s\n", depth += 2, "start ", name); } }
#define debugpi(name, param) { if (DEBUG_PARSER) { fprintf(stderr, "%*s%s: %d\n", depth += 2, "start ", name, param); } }
#define debugend(name) { if (DEBUG_PARSER) { fprintf(stderr, "%*s%s\n", depth -= 2, "end ", name); } }

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
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

typedef enum {
    TYPE_FUNCTION,
    TYPE_GLOBAL,
} FunctionType;

typedef struct Compiler {
    int localCount;
    int scopeDepth;
    int localsSize;
    ObjFunction* function;
    FunctionType type;
    struct Compiler* enclosing;
    struct Compiler* next;
    // This field must always be at the end
    Local locals[];
} Compiler;

Parser parser;
Compiler* current = NULL;
Compiler* root = NULL;

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
    if (!current->function) {
        return NULL;
    }
    return &current->function->chunk;
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
    emitByte(OP_NIL);
    emitByte(OP_RETURN);
}

static void emitBytes(uint8_t b1, uint8_t b2) {
    emitByte(b1);
    emitByte(b2);
}

static void emitNegJump(int jumpTo) {
    emitByte(OP_NEG_JUMP);
    int offset = currentChunk() ->count - jumpTo + SIZE_OF_24BIT_ARGS;
    if (offset > 1 << 24) {
        error("Too much jump!");
    }
    write24Bit(currentChunk(), offset, parser.previous.line, parser.previous.line);
}

static int emitJump(OpCode instr) {
    emitByte(instr);
    write24Bit(currentChunk(), (unsigned)-1, parser.previous.line, parser.previous.line);
    return currentChunk()->count - SIZE_OF_24BIT_ARGS;
}

static void patchJump(int offset) {
    int jump = currentChunk()->count - offset - 3;
    if (jump < 0) {
        error("Negative jump!");
    }
    if (jump > 1 << 24) {
        error("Too much jump!");
    }
    currentChunk()->code[offset + 0] = (uint8_t)((jump) & 0xff);
    currentChunk()->code[offset + 1] = (uint8_t)((jump >> 8) & 0xff);
    currentChunk()->code[offset + 2] = (uint8_t)((jump >> 16) & 0xff);
}

static int makeConstant(Value value) {
    int constant = addConstant(currentChunk(), value);
    return constant;
}

static int emitConstant(Value value) {
    return writeConstant(currentChunk(), value, parser.previous.line, parser.previous.column);
}

static void initCompiler(size_t localsSize, FunctionType type) {
    ObjString* name;
    if (type != TYPE_GLOBAL) {
        name = copyString(
            parser.previous.start,
            parser.previous.length
        );
    } else {
        name = copyString("<top_level>", sizeof("<top_level>"));
    }

    Compiler* compiler = calloc(sizeof(Compiler) + sizeof(Local) * localsSize, 1);
    compiler->enclosing = current;
    compiler->function = newFunction(name, NULL);
    compiler->type = type;
    compiler->localCount = 0;
    compiler->scopeDepth = 0;
    compiler->localsSize = localsSize;
    current = compiler;
    compiler->next = root;
    root = compiler;

    // Claim the first variable at the top of the stack for special reason
    Local* first = &current->locals[current->localCount++];
    first->depth = 0;
    first->name.start = "";
    first->name.length = 0;
}

static void freeCompiler(void) {
    Compiler* compiler = root;
    while (compiler != NULL) {
        Compiler* next = compiler->next;
        free(compiler);
        compiler = next;
    }

}

static void markInitialized(void) {
    if (current->scopeDepth == 0) return;
    current->locals[current->localCount - 1].depth = current->scopeDepth;
}

static void defineVariable(int global) {
    debugp("defineVariable");
    if (current->scopeDepth == 0) {
        writeConstantByOffset(currentChunk(), OP_DEFINE_GLOBAL, OP_DEFINE_GLOBAL_LONG, global, parser.previous.line, parser.previous.column);
    } else {
        markInitialized();
        // No op-codes needed to define local variables at runtime
    }
    debugend("defineVariable");
}

static ObjFunction* endCompiler(bool debugPrint) {
    emitReturn();
    ObjFunction* func = current->function;
    if (debugPrint) {
        // TODO filename and script directory
        const char* name = func->name ? func->name->chars : "<script>";
        disChunk(currentChunk(), name);
    }
    current = current->enclosing;
    return func;
}

static void beginScope(void) {
    current->scopeDepth++;
}

static void endScope(void) {
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
    debugp("binary");
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
        case TOKEN_COLON:
        case TOKEN_NAN:
        case TOKEN_INF:

        case TOKEN_LEFT_PAREN:
        case TOKEN_RIGHT_PAREN:
        case TOKEN_LEFT_BRACE:
        case TOKEN_RIGHT_BRACE:
        case TOKEN_LEFT_SQUARE_BRACE:
        case TOKEN_RIGHT_SQUARE_BRACE:
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

        case TOKEN_MINUS_EQUAL:
        case TOKEN_PLUS_EQUAL:
        case TOKEN_BITAND_EQUAL:
        case TOKEN_BITOR_EQUAL:
        case TOKEN_BITXOR_EQUAL:
        case TOKEN_LEFT_SHIFT_EQUAL:
        case TOKEN_RIGHT_SHIFT_EQUAL:
        case TOKEN_SLASH_EQUAL:
        case TOKEN_REMAINDER_EQUAL:
        case TOKEN_STAR_EQUAL:
        case TOKEN_STAR_STAR_EQUAL:

        case TOKEN_EOF:
            break;
    }
    debugp("binary");
}

static uint8_t argumentList() {
    uint8_t argCount = 0;
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            expression();
            if (argCount == 255) {
                error("Can't have more than 255 arguments.");
            }
            argCount++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return argCount;
}

static void call(bool canAssign) {
    uint8_t argCount = argumentList();
    emitBytes(OP_CALL, argCount);
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
    debugp("addLocal");
    Local* local = &current->locals[current->localCount++];
    local->name = name;
    // First, it starts uninitialized
    local->depth = -1;
    // TODO re-all if too many locals
    if (current->localCount == 1024) {
        error("Too many local variables.");
        return ;
    }
    debugend("addLocal");
}

static void declareVariable(void) {
    debugp("declareVariable");
    if (current->scopeDepth == 0) {
        debugend("declareVariable");
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
    debugend("declareVariable");
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

static void expression(void) {
    debugp("expression");
    // Vaughan Pratt's top-down operator precedence parsing
    // Parse an expression starting at the lowest precedence levels
    parsePrecedence(PREC_ASSIGNMENT);
    debugend("expression");
}

static void block(void) {
    debugp("block");
    while (!(check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))) {
        declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
    debugend("block");
}

static void function(FunctionType type) {
    debugp("function");
    initCompiler(256, type);
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function definition.");
    if (!check(TOKEN_RIGHT_PAREN)) {
        do {
            current->function->arity++;
            uint8_t constant = parseVariable("Expect parameter name.");
            // TODO parse function paramNames and documentation
            defineVariable(constant);
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after function definition.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();
    ObjFunction* function = endCompiler(DEBUG_TRACE);
    emitBytes(OP_CONSTANT, makeConstant(OBJ_VAL(function)));
    debugend("function");
}

static void funDeclaration(void) {
    debugp("funDeclaration");
    uint8_t global = parseVariable("Expect function name.");
    markInitialized();
    function(TYPE_FUNCTION);
    defineVariable(global);
    debugend("funDeclaration");
}

static void varDeclaration(void) {
    debugp("varDeclaration");
    int global = parseVariable("Expect variable name.");
    if (match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration");
    defineVariable(global);
    debugend("varDeclaration");
}

static void printStatement(void) {
    debugp("printStatement");
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after print.");
    emitByte(OP_PRINT);
    debugend("printStatement");
}

static void returnStatement(void) {
    if (current->type == TYPE_GLOBAL) {
        error("Can't return from top-level code.");
    }
    if (match(TOKEN_SEMICOLON)) {
        emitReturn();
    } else {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after return.");
        emitByte(OP_RETURN);
    }
}

static void whileStatement(void) {
    debugp("whileStatement");
    int loopStart = currentChunk()->count;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'while'.");
    int exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    emitNegJump(loopStart);
    patchJump(exitJump);
    emitByte(OP_POP);
    debugend("whileStatement");
}

static void expressionStatement(void) {
    debugp("expressionStatement");
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after print.");
    emitByte(OP_POP);
    debugend("expressionStatement");
}

static void forStatement(void) {
    debugp("forStatement");
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    if (match(TOKEN_SEMICOLON)) {
        // No initializer
    } else if (match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        expressionStatement();
    }
    int loopStart = currentChunk()->count;
    int exitJump = -1;
    if (!match(TOKEN_SEMICOLON)) {
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        exitJump = emitJump(OP_JUMP_IF_FALSE);
        emitByte(OP_POP);
    }
    if (!match(TOKEN_RIGHT_PAREN)) {
        int bodyJump = emitJump(OP_JUMP);
        int incrementStart = currentChunk()->count;
        expression();
        emitByte(OP_POP);
        consume(TOKEN_RIGHT_PAREN, "Expect ')' after 'if'.");
        emitNegJump(loopStart);
        loopStart = incrementStart;
        patchJump(bodyJump);
    }
    statement();
    emitNegJump(loopStart);
    if (exitJump != -1) {
        patchJump(exitJump);
        emitByte(OP_POP);
    }
    endScope();
    debugend("forStatement");
}

static void ifStatement(void) {
    debugp("ifStatement");
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect '(' after 'if'.");
    int thenJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    statement();
    int elseJump = emitJump(OP_JUMP);
    patchJump(thenJump);
    emitByte(OP_POP);
    if (match(TOKEN_ELSE)) {
        statement();
    }
    patchJump(elseJump);
    debugend("ifStatement");
}

static void statement(void) {
    debugp("statement");
    if (match(TOKEN_PRINT)) {
        printStatement();
    } else if (match(TOKEN_IF)) {
        ifStatement();
    } else if (match(TOKEN_WHILE)) {
        whileStatement();
    } else if (match(TOKEN_FOR)) {
        forStatement();
    } else if (match(TOKEN_RETURN)) {
        returnStatement();
    } else if (match(TOKEN_LEFT_BRACE)) {
        beginScope();
        block();
        endScope();
    } else {
        expressionStatement();
    }
    debugend("statement");
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
    } else if (match(TOKEN_FUN)) {
        funDeclaration();
    } else {
        statement();
    }
    if (parser.panicMode) {
        synchronize();
    }
}

static void grouping(bool canAssign) {
    debugp("grouping");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect end ')' after expression.");
    debugend("grouping");
}

static void unary(bool canAssign) {
    debugp("unary");
    TokenType opType = parser.previous.type;
    parsePrecedence(PREC_UNARY);
    switch (opType) {
        case TOKEN_MINUS: emitByte(OP_NEG); break;
        case TOKEN_PLUS: break;
        case TOKEN_SIZE: emitByte(OP_SIZE); break;
        case TOKEN_BITNEG: emitByte(OP_BITNEG); break;
        case TOKEN_BANG: emitByte(OP_NOT); break;

        default:
            break;
    }
    debugend("unary");
}

static void literal(bool canAssign) {
    debugp("literal");
    switch (parser.previous.type) {
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        case TOKEN_NAN: emitByte(OP_NAN); break;
        case TOKEN_INF: emitByte(OP_INF); break;
        default:
            break;
    }
    debugend("literal");
}

static void string(bool canAssign) {
    debugp("string");
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));
    debugend("string");
}

static int resolveLocal(Compiler* compiler, Token* name) {
    for (int i = compiler->localCount - 1; i >= 0; i--) {
        Local local = compiler->locals[i];
        if (identifiersEqual(name, &local.name)) {
            if (local.depth == -1) {
                error("Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

static void namedVariable(Token name, bool canAssign) {
    int offset = resolveLocal(current, &name);

    OpCode instr, instrLong;
    if (canAssign && 
        (match(TOKEN_EQUAL)
        || match(TOKEN_MINUS_EQUAL)
        || match(TOKEN_PLUS_EQUAL)
        || match(TOKEN_BITAND_EQUAL)
        || match(TOKEN_BITOR_EQUAL)
        || match(TOKEN_BITXOR_EQUAL)
        || match(TOKEN_SLASH_EQUAL)
        || match(TOKEN_REMAINDER_EQUAL)
        || match(TOKEN_STAR_EQUAL)
        || match(TOKEN_STAR_STAR_EQUAL)
        || match(TOKEN_LEFT_SHIFT_EQUAL)
        || match(TOKEN_RIGHT_SHIFT_EQUAL))
    ) {
        TokenType opType = parser.previous.type;
        // Step (1): Get current value if needed
        if (opType != TOKEN_EQUAL) {
            //////////// TODO this code is horrendous 
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
            writeConstantByOffset(currentChunk(), instr, instrLong, offset, parser.previous.line, parser.previous.column);
            if (instr == OP_GET_GLOBAL) {
                offset = -1;
            }
        }
        // Step (2): Get new value
        expression();
        // Step (3): Mutate current value using new value if needed
        switch (opType) {
            case TOKEN_PLUS_EQUAL: emitByte(OP_ADD); break;
            case TOKEN_MINUS_EQUAL: emitByte(OP_SUB); break;
            case TOKEN_STAR_EQUAL: emitByte(OP_MUL); break;
            case TOKEN_SLASH_EQUAL: emitByte(OP_DIV); break;
            case TOKEN_STAR_STAR_EQUAL: emitByte(OP_EXP); break;
            case TOKEN_REMAINDER_EQUAL: emitByte(OP_REMAINDER); break;
            case TOKEN_BITAND_EQUAL: emitByte(OP_BITAND); break;
            case TOKEN_BITOR_EQUAL: emitByte(OP_BITOR); break;
            case TOKEN_BITXOR_EQUAL: emitByte(OP_BITXOR); break;
            case TOKEN_LEFT_SHIFT_EQUAL: emitByte(OP_LEFT_SHIFT); break;
            case TOKEN_RIGHT_SHIFT_EQUAL: emitByte(OP_RIGHT_SHIFT); break;
            case TOKEN_EQUAL:
            default:
                break;
        }
        // Step (4): Save new value or mutated value to variable
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
    debugp("variable");
    namedVariable(parser.previous, canAssign);
    debugend("variable");
}

static void number(bool canAssign) {
    debugp("number");
    double value = strtod(parser.previous.start, NULL);
    emitConstant(DOUBLE_VAL(value));
    debugend("number");
}

static void integer(bool canAssign) {
    debugp("integer");
    int value = strtol(parser.previous.start, NULL, 10);
    emitConstant(INTEGER_VAL(value));
    debugend("integer");
}

static void hexnumber(bool canAssign) {
    debugp("hexnumber");
    int value = strtol(parser.previous.start, NULL, 16);
    emitConstant(INTEGER_VAL(value));
    debugend("hexnumber");
}

static void and_(bool canAssign) {
    debugp("and_");
    // AND can short-circuit, so it uses jumps
    int endJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    parsePrecedence(PREC_AND);
    patchJump(endJump);
    debugend("and_");
}

static void or_(bool canAssign) {
    debugp("or_");
    // OR can short-circuit, so it uses jumps
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    int endJump = emitJump(OP_JUMP);
    patchJump(elseJump);
    emitByte(OP_POP);
    parsePrecedence(PREC_OR);
    patchJump(endJump);
    debugend("or_");
}

static void array(bool canAssign) {
    debugp("array");
    // Array literal
    emitByte(OP_INIT_ARRAY);
    while (!(check(TOKEN_RIGHT_SQUARE_BRACE) && !check(TOKEN_EOF))) {
        expression();
        if (!check(TOKEN_RIGHT_SQUARE_BRACE)) {
            consume(TOKEN_COMMA, "Expect ',' after array element");
        } else {
            // Optional comma at end of list
            match(TOKEN_COMMA);
        }
        emitByte(OP_INSERT_ARRAY);
    }
    consume(TOKEN_RIGHT_SQUARE_BRACE, "Expect ']' at end of array.");
    debugend("array");
}

// This pushes either an array (slice) or a single value (index)
static void subscript(bool canAssign) {
    debugp("subscript");
    bool isArray = false;
    // (1) First, we need at least one value on the stack
    if (match(TOKEN_COLON)) {
        // (1.1) If it starts with colon, it's a slice equivalent to [0:...]
        emitConstant(INTEGER_VAL(0));
        isArray = true;
    } else {
        // (1.2) Otherwise, it might be a simple index
        //       or a slice that starts with an integer
        expression();
    }
    // (2) Are we in a slice or not?
    if (match(TOKEN_COLON)) {
        isArray = true;
    }
    // (3) Slice detected! One constant on stack is guaranteed
    if (isArray) {
        // (3.1) Push new array
        emitByte(OP_INIT_ARRAY);
        // (3.2) Then insert the value below the array
        emitByte(OP_SWAP);
        emitByte(OP_INSERT_ARRAY);
    }
    // (4) Now parse any remaining components of a slice
    while (!(check(TOKEN_RIGHT_SQUARE_BRACE) && !check(TOKEN_EOF))) {
        expression();
        // (4.1) Keep pushing indices into the slice array
        emitByte(OP_INSERT_ARRAY);
        if (!check(TOKEN_RIGHT_SQUARE_BRACE)) {
            // (4.2) A slice may or may not end in a colon
            consume(TOKEN_COLON, "Expect ':' in array slice.");
        }
    }
    consume(TOKEN_RIGHT_SQUARE_BRACE, "Expect ']' after array subscript or slice.");
    emitByte(OP_SUBSCRIPT);
    debugend("subscript");
}

static void hashmap(bool canAssign) {
    debugp("hashmap");
    emitByte(OP_INIT_HASHMAP);
    while (!(check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF))) {
        expression();
        consume(TOKEN_COLON, "Expect ':' after hashmap key");
        expression();
        if (!check(TOKEN_RIGHT_BRACE)) {
            consume(TOKEN_COMMA, "Expect ',' after hashmap element");
        } else {
            // Optional comma at end of list
            match(TOKEN_COMMA);
        }
        emitByte(OP_INSERT_HASHMAP);
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' at end of hashmap.");
    debugend("hashmap");
}


ParseRule rules[] = {
    // TODO exhaustive
    [TOKEN_NAN]                = {literal, NULL, PREC_NONE},
    [TOKEN_INF]                = {literal, NULL, PREC_NONE},
    [TOKEN_COLON]              = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_PAREN]         = {grouping, call, PREC_CALL},
    [TOKEN_RIGHT_PAREN]        = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE]         = {hashmap, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE]        = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_SQUARE_BRACE]  = {array, subscript, PREC_CALL},
    [TOKEN_RIGHT_SQUARE_BRACE] = {NULL, NULL, PREC_NONE},
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
    [TOKEN_MINUS_EQUAL]        = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_PLUS_EQUAL]         = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_BITAND_EQUAL]       = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_BITOR_EQUAL]        = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_BITXOR_EQUAL]       = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_LEFT_SHIFT_EQUAL]   = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_RIGHT_SHIFT_EQUAL]  = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_SLASH_EQUAL]        = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_REMAINDER_EQUAL]    = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_STAR_EQUAL]         = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_STAR_STAR_EQUAL]    = {NULL, NULL, PREC_EQUALITY},
    [TOKEN_STRING]             = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER]             = {number, NULL, PREC_NONE},
    [TOKEN_INTEGER]            = {integer, NULL, PREC_NONE},
    [TOKEN_HEXINT]             = {hexnumber, NULL, PREC_NONE},
    [TOKEN_AND]                = {NULL, and_, PREC_AND},
    [TOKEN_CLASS]              = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE]               = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE]              = {literal, NULL, PREC_NONE},
    [TOKEN_FOR]                = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN]                = {NULL, NULL, PREC_NONE},
    [TOKEN_IF]                 = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL]                = {literal, NULL, PREC_NONE},
    [TOKEN_OR]                 = {NULL, or_, PREC_OR},
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
    debugpi("parsePrecedence", precedence);
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
    debugend("parsePrecedence");
}

ObjFunction* compile(const char* source) {
    if (DEBUG_TRACE) {
        DEBUG_PARSER = true;
    }
    parser.hadError = false;
    parser.panicMode = false;
    initScanner(source);
    initCompiler(256, TYPE_GLOBAL);
    advance();
    while (!match(TOKEN_EOF)) {
        declaration();
    }
    consume(TOKEN_EOF, "Expect end of expression.");
    ObjFunction* func = endCompiler(DEBUG_TRACE);
    freeCompiler();
    return parser.hadError ? NULL : func;
}
