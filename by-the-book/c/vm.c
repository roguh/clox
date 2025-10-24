#include <stdio.h>

#include "common.h"
#include "debug.h"
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

Value top() {
    return *(vm.stackTop - 1);
}

size_t size() {
    return vm.stackTop - vm.stack;
}

static InterpretResult run() {
#define READ_BYTE() (vm.chunk->code[vm.ip++])
#define READ_CONSTANT() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (vm.chunk->constants.values[READ_BYTE() | READ_BYTE() << 8 | READ_BYTE() << 16])
#define BIN_OP(op) do { double b = pop() ; double a = pop() ; push(a op b); } while (false)

    while (true) {
        if (DEBUG_TRACE) {
        disInstruction(vm.chunk, vm.ip);
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printf("[ ");
                printValue(top());
                printf(" ]");
            }
            printf("\n");
        }
        uint8_t instruction;
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
            case OP_CONSTANT: {
                Value constant = READ_CONSTANT();
                push(constant);
                break;
            }
            case OP_CONSTANT_LONG: {
                Value constant = READ_CONSTANT_LONG();
                push(constant);
                break;
            }
            case OP_NEG: {
                Value val = pop();
                push(-val);
                break;
            }
            case OP_ADD: BIN_OP(+); break;
            case OP_SUB: BIN_OP(-); break;
            case OP_MUL: BIN_OP(*); break;
            case OP_DIV: BIN_OP(/); break;
        }
    }
}

InterpretResult interpret(Chunk* chunk) {
    vm.chunk = chunk;
    vm.ip = 0;
    return run();
}

InterpretResult interpretString(const char* program) {
    printf("%s", program);
    return INTERPRET_OK;
}
