#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "object.h"
#include "memory.h"
#include "value.h"
#include "vm.h"

VM vm;

static void resetStack(void) {
    vm.stackTop = vm.stack;
}

void initVM(void) {
    resetStack();
    hashmap_init(&vm.strings, 1024, (hash_function)hashAny);
    hashmap_init(&vm.globals, 32, (hash_function)hashAny);
    vm.objects = NULL;
}

void freeVM(void) {
    hashmap_free(&vm.strings);
    hashmap_free(&vm.globals);
    freeObjects();
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop(void) {
    vm.stackTop--;
    return *vm.stackTop;
}

int pop_int(void) {
    return AS_INTEGER(pop());
}

double pop_double(void) {
    return AS_DOUBLE(pop());
}

static bool isFalsey(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

Value peek(int offset) {
    return vm.stackTop[-(offset + 1)];
}

static void runtimeError(const char* format, ...) {
    va_list args;
    fputs("ERROR: ", stderr);
    va_start(args, format);
    vfprintf(stderr,  format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t instruction = vm.chunk->code[vm.ip]; // vm.ip-1 ? TODO
    int line = vm.chunk->lines[instruction];
    int col = vm.chunk->columns[instruction];
    fprintf(stderr, "    [%d:%d] in script\n", line, col);
    resetStack();
}

size_t size(void) {
    return vm.stackTop - vm.stack;
}

static void concatenate(void) {
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());
    size_t length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';
    ObjString* result = copyString(chars, length);
    free(chars);
    push(OBJ_VAL(result));
}

static InterpretResult run(void) {
#pragma GCC diagnostic ignored "-Wsequence-point"
#define READ_BYTE() (vm.chunk->code[vm.ip++])

#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_24BITS() (READ_BYTE() | READ_BYTE() << 8 | READ_BYTE() << 16)
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_24BITS()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())

#define BIN_OP(op, _if_double, _if_int) do { \
    Value b = pop(); \
    Value a = pop(); \
    push(IS_DOUBLE(a) || IS_DOUBLE(b) ? \
        _if_double(AS_DOUBLE(a) op AS_DOUBLE(b)) : \
        _if_int(AS_INTEGER(a) op AS_INTEGER(b))); \
} while (false)

#define INT_BIN_OP(op) do { \
    int b = pop_int(); \
    int a = pop_int(); \
    push(INTEGER_VAL(a op b)); \
} while (false)

#define DOUBLE_BIN_OP(func) do { \
    double b = pop_double(); \
    double a = pop_double(); \
    push(DOUBLE_VAL(func(a, b))); \
} while (false)

    while (true) {
        if (DEBUG_TRACE) {
            printf("[ ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printValue(*slot);
                if (slot < vm.stackTop - 1) {
                    printf(" ");
                }
            }
            printf(" ]\n");
            disInstruction(vm.chunk, vm.ip);
        }
        OpCode instruction;
        switch (instruction = READ_BYTE()) { // This switch is exhaustive!
            case OP_RETURN: return INTERPRET_OK;
            case OP_PRINT: {
                if (size()) {
                    printValue(pop());
                    printf("\n");
                }
                break;
            }
            case OP_POP: 
                if (size()) {
                    pop();
                }
                break;
            case OP_DEFINE_GLOBAL:
            case OP_DEFINE_GLOBAL_LONG: {
                ObjString* name = (instruction == OP_DEFINE_GLOBAL) ? READ_STRING() : READ_STRING_LONG();
                hashmap_add(&vm.globals, OBJ_VAL(name), pop());
                break;
            }
            case OP_SET_GLOBAL_LONG:
            case OP_SET_GLOBAL: {
                ObjString* name = (instruction == OP_SET_GLOBAL) ? READ_STRING() : READ_STRING_LONG();
                if (!hashmap_set(&vm.globals, OBJ_VAL(name), peek(0))) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_GLOBAL_LONG:
            case OP_GET_GLOBAL: {
                ObjString* name = (instruction == OP_GET_GLOBAL) ? READ_STRING() : READ_STRING_LONG();
                bool notFound = false;
                Value value = hashmap_get(&vm.globals, OBJ_VAL(name), &notFound);
                if (notFound) {
                    runtimeError("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_SET_LOCAL_LONG:
            case OP_SET_LOCAL: {
                int slot = (instruction == OP_SET_LOCAL) ? READ_BYTE() : READ_24BITS();
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_GET_LOCAL_LONG:
            case OP_GET_LOCAL: {
                int slot = (instruction == OP_GET_LOCAL) ? READ_BYTE() : READ_24BITS();
                push(vm.stack[slot]);
                break;
            }
            case OP_EQUAL: {
                push(BOOL_VAL(valuesEqual(pop(), pop())));
                break;
            }
            case OP_CONSTANT: push(READ_CONSTANT()); break;
            case OP_CONSTANT_LONG: push(READ_CONSTANT_LONG()); break;
            case OP_NEG: push(DOUBLE_VAL(-pop_double())); break;
            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_BITNEG: push(INTEGER_VAL(~pop_int())); break;
            case OP_SIZE: if (IS_STRING(peek(0))) {
                push(INTEGER_VAL(strlen(AS_CSTRING(pop()))));
            } else {
                push(INTEGER_VAL(sizeof(Value)));
            }
            break;
            case OP_GREATER: BIN_OP(>, BOOL_VAL, BOOL_VAL); break;
            case OP_LESS: BIN_OP(<, BOOL_VAL, BOOL_VAL); break;
            case OP_ADD: {
                if (IS_STRING(peek(0)) || IS_STRING(peek(1))) {
                    if (!(IS_STRING(peek(0)) && IS_STRING(peek(1)))) {
                        runtimeError("Strings can only be added to other strings");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    concatenate();
                } else {
                    BIN_OP(+, DOUBLE_VAL, INTEGER_VAL);
                }
                break;
            }
            case OP_SUB: BIN_OP(-, DOUBLE_VAL, INTEGER_VAL); break;
            case OP_MUL: BIN_OP(*, DOUBLE_VAL, INTEGER_VAL); break;
            case OP_DIV: BIN_OP(/, DOUBLE_VAL, INTEGER_VAL); break;
            case OP_BITAND: INT_BIN_OP(&); break;
            case OP_BITOR: INT_BIN_OP(|); break;
            case OP_BITXOR: INT_BIN_OP(^); break;
            case OP_LEFT_SHIFT: INT_BIN_OP(<<); break;
            case OP_RIGHT_SHIFT: INT_BIN_OP(>>); break;
            case OP_REMAINDER: DOUBLE_BIN_OP(fmod); break;
            case OP_EXP: DOUBLE_BIN_OP(pow); break;
            case OP_NIL: push(NIL_VAL); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
        }
    }
}

InterpretResult interpretChunk(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = 0;
    if (DEBUG_TRACE) {
        printf("== running! ==\n");
    }
    InterpretResult result = run();
    freeVM();
    return result;
}

InterpretResult interpret(const char* program) {
    initVM();
    Chunk chunk;
    initChunk(&chunk);
    if (!compile(program, &chunk, DEBUG_TRACE)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    InterpretResult result = interpretChunk(&chunk);
    freeChunk(&chunk);
    return result;
}

InterpretResult interpretStream(const char* program, Chunk* chunk) {
    if (!compile(program, chunk, DEBUG_TRACE)) {
        freeChunk(chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    InterpretResult result = interpretChunk(chunk);
    return result;
}
