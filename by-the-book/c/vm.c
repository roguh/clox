#include <stdio.h>
#include <math.h>

#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "value.h"
#include "vm.h"

VM vm;

static void resetStack() {
    vm.stackTop = vm.stack;
}

void initVM() {
    resetStack();
}

void freeVM() {
}

void push(Value value) {
    *vm.stackTop = value;
    vm.stackTop++;
}

Value pop() {
    vm.stackTop--;
    return *vm.stackTop;
}

int pop_int() {
    return AS_INTEGER(pop());
}

double pop_double() {
    return AS_DOUBLE(pop());
}

Value top() {
    return *(vm.stackTop - 1);
}

size_t size() {
    return vm.stackTop - vm.stack;
}

static InterpretResult run() {
#pragma GCC diagnostic ignored "-Wsequence-point"
#define READ_BYTE() (vm.chunk->code[vm.ip++])

#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_BYTE() | READ_BYTE() << 8 | READ_BYTE() << 16])
#define BIN_OP(op) do { \
    Value b = pop(); \
    Value a = pop(); \
    push(IS_DOUBLE(a) || IS_DOUBLE(b) ? \
        DOUBLE_VAL(AS_DOUBLE(a) op AS_DOUBLE(b)) : \
        INTEGER_VAL(AS_INTEGER(a) op AS_INTEGER(b))); \
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
        disInstruction(vm.chunk, vm.ip);
            printf("[ ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printValue(top());
                if (slot < vm.stackTop - 1) {
                    printf(" ");
                }
            }
            printf(" ]\n");
        }
        OpCode instruction;
        // This switch is exhaustive!
        switch (instruction = READ_BYTE()) {
            case OP_RETURN: {
                if (size()) {
                    printValue(pop());
                    printf("\n");
                }
                return INTERPRET_OK;
            }
            case OP_PRINT: {
                if (size()) {
                    printValue(top());
                    printf("\n");
                }
                break;
            }
            case OP_CONSTANT: push(READ_CONSTANT()); break;
            case OP_CONSTANT_LONG: push(READ_CONSTANT_LONG()); break;
            case OP_NEG: push(DOUBLE_VAL(-pop_double())); break;
            case OP_BITNEG: push(INTEGER_VAL(~pop_int())); break;
            case OP_SIZE: push(INTEGER_VAL(sizeof(Value))); break;
            case OP_ADD: BIN_OP(+); break;
            case OP_SUB: BIN_OP(-); break;
            case OP_BITAND: INT_BIN_OP(&); break;
            case OP_BITOR: INT_BIN_OP(|); break;
            case OP_BITXOR: INT_BIN_OP(^); break;
            case OP_LEFT_SHIFT: INT_BIN_OP(<<); break;
            case OP_RIGHT_SHIFT: INT_BIN_OP(>>); break;
            case OP_MUL: BIN_OP(*); break;
            case OP_DIV: BIN_OP(/); break;
            case OP_REMAINDER: DOUBLE_BIN_OP(fmod); break;
            case OP_EXP: DOUBLE_BIN_OP(pow); break;
            case OP_NIL: push(NIL_VAL); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
        }
    }
}

InterpretResult interpretChunk(Chunk* chunk) {
    initVM();
    vm.chunk = chunk;
    vm.ip = 0;
    if (DEBUG_TRACE) {
        printf("Running!\n");
    }
    InterpretResult result = run();
    freeVM();
    return result;
}

InterpretResult interpret(const char* program) {
    Chunk chunk;
    initChunk(&chunk);
    if (!compile(program, &chunk, true)) {
        freeChunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    InterpretResult result = interpretChunk(&chunk);
    freeChunk(&chunk);
    return result;
}
