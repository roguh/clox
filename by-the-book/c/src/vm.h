#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"
#include "object.h"
#include "hashmap.h"

#define FRAMES_MAX 256
#define STACK_MAX (FRAMES_MAX * UINT8_COUNT)

typedef struct {
    ObjFunction* function;
    uint8_t* ip;
    Value* slots;
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frameCount;
    Value stack[STACK_MAX];
    Value* stackTop;
    Obj* objects;
    ObjHashmap* globals;
    ObjHashmap* strings;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
} InterpretResult;

extern VM vm;

#define runtimeError(...) { runtimeErrorLog(__VA_ARGS__); dangerousUnsafeResetStack(); }

void initVM(void);
void freeVM(void);
InterpretResult interpretOrPrint(const char* string, bool onlyPrint);
InterpretResult interpret(const char* string);
InterpretResult interpretChunk(Chunk* chunk);
void push(Value value);
Value pop(void);

void defineNative(const char* name, int arity, NativeFn function);
void defineConstant(const char* name, Value val);
void runtimeErrorLog(const char* format, ...);
void dangerousUnsafeResetStack(void);

#endif
