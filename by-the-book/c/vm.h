#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 1024

typedef struct {
    Chunk* chunk;
    size_t ip;
    Value stack[STACK_MAX];
    Value* stackTop;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

void initVM();
void freeVM();
InterpretResult interpret(const char* string);
InterpretResult interpretChunk(Chunk* chunk);
void push(Value value);
Value pop();
bool valuesEqual(Value a, Value b);

#endif
