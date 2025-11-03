#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "common.h"
#include "debug.h"
#include "compiler.h"
#include "hashmap.h"
#include "object.h"
#include "memory.h"
#include "value.h"
#include "vm.h"
#include "print.h"
#include "lib_complex.h"

VM vm;

#define runtimeError(...) { runtimeErrorLog(__VA_ARGS__); resetStack(); }

static Value clockNative(int argCount, Value* args) {
    return DOUBLE_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value lineNative(int argCount, Value* args) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    int line = frame->function->chunk.lines[frame->ip - frame->function->chunk.code - 1];
    return INTEGER_VAL(line);
}

static Value colNative(int argCount, Value* args) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    int col = frame->function->chunk.columns[frame->ip - frame->function->chunk.code - 1];
    return INTEGER_VAL(col);
}

static void resetStack(void) {
    vm.stackTop = vm.stack;
}

static void defineNative(const char* name, int arity, NativeFn function) {
    ObjString* _name = copyString(name, strlen(name));
    hashmap_add(
        &vm.globals,
        OBJ_VAL(_name),
        OBJ_VAL(newNative(_name, arity, function))
    );
}

static Value FFI_prints(int argCount, Value* values) {
    for (int i = 0; i < argCount; i++) {
        printValue(values[i]);
    }
    return NIL_VAL;
}

static Value FFI_setArray(int argCount, Value* arg) {
    insertArray(AS_ARRAY(arg[0]), AS_INTEGER(arg[1]), arg[2]);
    return NIL_VAL;
}

static Value FFI_rmArrayTop(int argCount, Value* arg) {
    printf("%d->", ARRAY_LENGTH(arg[0]));
    AS_ARRAY(arg[0])->length--;
    printf("%d\n", ARRAY_LENGTH(arg[0]));
    return NIL_VAL;
}

void initVM(void) {
    // vm.frameCount
    vm.frameCount = 0;
    // vm.stack and vm.stackTop
    resetStack();
    // vm.objects
    vm.objects = NULL;
    // vm.globals
    /////// TODO test with many globals
    hashmap_init(&vm.globals, 512, (hash_function)hashAny);
    // vm.strings
    hashmap_init(&vm.strings, 1024, (hash_function)hashAny);

    // Put these AFTER defining VM
    defineNative("clock", 0, clockNative);
    defineNative("__line__", 0, lineNative);
    defineNative("__col__", 0, colNative);
    defineNative("prints", -1, FFI_prints);
    defineNative("setArray", 3, FFI_setArray);
    defineNative("rmArrayTop", 1, FFI_rmArrayTop);

    defineComplexLib();
}

void freeVM(void) {
    hashmap_free(&vm.strings);
    hashmap_free(&vm.globals);
    freeObjects();
}

static void runtimeErrorLog(const char* format, ...) {
    va_list args;
    fputs("ERROR: ", stderr);
    va_start(args, format);
    vfprintf(stderr,  format, args);
    va_end(args);
    fputs("\n", stderr);

    for (int i = vm.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &vm.frames[i];
        int line = frame->function->chunk.lines[frame->ip - frame->function->chunk.code - 1];
        int col = frame->function->chunk.columns[frame->ip - frame->function->chunk.code - 1];
        fprintf(stderr, "    [%d:%d] in %s\n", line, col, frame->function->name->chars);
    }
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

#define ARITY_CHECK(func) \
    if (func->arity >= 0 && argCount != func->arity) { \
        runtimeError("%s() expected %d arguments but got %d.", func->name->chars, func->arity, argCount); \
        return false; \
    }

static bool call(ObjFunction* func, int argCount) {
    ARITY_CHECK(func)
    if (vm.frameCount == FRAMES_MAX) {
        runtimeError("Stack overflow.");
        return false;
    }
    vm.frameCount += 1;
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    frame->function = func;
    frame->ip = func->chunk.code;
    frame->slots = vm.stackTop - argCount - 1;
    return true;
}

static bool callValue(Value callee, int argCount) {
    if (IS_OBJ(callee)) {
        switch (OBJ_TYPE(callee)) {
            case OBJ_FUNCTION:
                return call(AS_FUNCTION(callee), argCount);
            case OBJ_NATIVE: {
                ObjNative* func = AS_NATIVE(callee);
                ARITY_CHECK(func)
                Value result = func->function(argCount, vm.stackTop - argCount);
                vm.stackTop -= argCount + 1;
                push(result);
                return true;
            }
            default: // Safe to automatically assume all other types are no callable
                break;
        }
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

static bool slice(Value key) {
    if (!IS_ARRAY(key)) {
        runtimeError("Invalid array slice");
        return false;
    }
    ObjArray* arr = AS_ARRAY(key);
    ObjType ty = AS_OBJ(peek(0))->type;
    switch (ty) {
        case OBJ_STRING_VIEW: break;
        case OBJ_STRING: {
            ObjString* string = AS_STRING(pop());

    int start = 0;
    int end = string->length;
    if (arr->length == 0) {
    } else if (arr->length == 1) {
        start = AS_INTEGER(AS_ARRAY(key)->values[0]);
    } else if (arr->length == 2) {
        start = AS_INTEGER(AS_ARRAY(key)->values[0]);
        end = AS_INTEGER(AS_ARRAY(key)->values[1]);
    } else {
        runtimeError("Cannot slice with more than two indices");
        return false;
    }

            if (end < 0) {
                end = string->length - (-end);
                if (end < 0) {
                    end = 0;
                }
            }
            if (start > end || start > string->length) {
                end = 0;
                start = 0;
            }
            push(OBJ_VAL(getStringView(string, start, end - start)));
            return true;
        }
        case OBJ_ARRAY: {
            return true;
        }
        case OBJ_HASHMAP:
            runtimeError("Cannot slice into hashmap yet");
            return false;
        case OBJ_NEVER:
            runtimeError("Slicing into a non-initialized object");
            return false;
        case OBJ_FUNCTION:
        case OBJ_NATIVE:
            runtimeError("Indexing into a non-array, non-string, non-hashmap value");
            return false;
    }
    runtimeError("Unreachable code in slice(), unexpected object?");
    return false;
}

static bool subscript(Value key) {
    if (IS_ARRAY(key)) {
        return slice(key);
    }
    if (IS_HASHMAP(peek(0))) {
        ObjHashmap* hm = AS_HASHMAP(pop());
        push(hashmap_get(&hm->map, key, NULL));
        return true;
    }
    if (!(IS_INTEGER(key) || IS_ARRAY(key))) {
        runtimeError("Array index must be an integer or a slice");
        return false;
    }
    int i = AS_INTEGER(key);
    ObjType ty = AS_OBJ(peek(0))->type;
    switch (ty) {
        case OBJ_STRING:
        case OBJ_STRING_VIEW: {
            ObjString* string = AS_STRING(pop());
            if (i < 0) {
                i = string->length + i;
            }
            if (i < 0 || i >= string->length) {
                runtimeError("String index %d out of bounds", i);
                return false;
            }
            push(OBJ_VAL(copyString(&string->chars[i], 1)));
            return true;
        }
        case OBJ_ARRAY: {
            ObjArray* array = AS_ARRAY(pop());
            if (i < 0) {
                i = array->length + i;
            }
            if (i < 0 || i >= array->length) {
                runtimeError("Array index %d out of bounds", i);
                return false;
            }
            push(array->values[i]);
            return true;
        }
        case OBJ_HASHMAP:
            return true;
        case OBJ_NEVER:
            runtimeError("Indexing into a non-initialized object");
            return false;
        case OBJ_FUNCTION:
        case OBJ_NATIVE:
            runtimeError("Indexing into a non-array, non-string, non-hashmap value");
            return false;
    }
    runtimeError("Unreachable code in subscript(), unexpected object?");
    return false;
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

static void concatenateArrays(void) {
    ObjArray* b = AS_ARRAY(pop());
    ObjArray* a = AS_ARRAY(pop());
    size_t length = a->length + b->length;
    // TODO make capacity a power of 2
    ObjArray* result = allocateArray(length);
    memcpy(result->values, a->values, a->length * sizeof(Value));
    memcpy(result->values + a->length, b->values, b->length * sizeof(Value));
    result->length = length;
    push(OBJ_VAL(result));
}

static void hashmap_err_print_key(hashmap_t* hm, unsigned long index, Value key, Value val, void* data) {
    // TODO print to stderr
    printValue(key);
    printf(" ");
}

static InterpretResult run(void) {
    // All on stack, no indirection. TODO cool?
    CallFrame* frame = &vm.frames[vm.frameCount - 1];

#pragma GCC diagnostic ignored "-Wsequence-point"
#define READ_BYTE() (*frame->ip++)
#define READ_24BITS() (READ_BYTE() | READ_BYTE() << 8 | READ_BYTE() << 16)
#define READ_CONSTANT() (frame->function->chunk.constants.values[READ_BYTE()])
#define READ_CONSTANT_LONG() (frame->function->chunk.constants.values[READ_24BITS()])
#define READ_STRING() AS_STRING(READ_CONSTANT())
#define READ_STRING_LONG() AS_STRING(READ_CONSTANT_LONG())

#define ARITH_BIN_OP(op) do { \
    Value b = pop(); \
    Value a = pop(); \
    push(IS_FCOMPLEX(a) || IS_FCOMPLEX(b) \
      ? FCOMPLEX_VAL(AS_FCOMPLEX(a) op AS_FCOMPLEX(b)) \
      : IS_DOUBLE(a) || IS_DOUBLE(b) ? \
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
            printf("[ ");
            for (Value* slot = vm.stack; slot < vm.stackTop; slot++) {
                printValue(*slot);
                if (slot < vm.stackTop - 1) {
                    printf(" ");
                }
            }
            printf(" ]\n");
            disInstruction(&frame->function->chunk, frame->ip - frame->function->chunk.code);
        }
        OpCode instruction;
        switch (instruction = READ_BYTE()) { // This switch is exhaustive!
            case OP_INVALID: {
                runtimeError("Unexpected null instruction!");
                return INTERPRET_RUNTIME_ERROR;
            }
            case OP_RETURN: {
                Value result = pop();
                vm.frameCount--;
                if (vm.frameCount == 0) {
                    pop();
                    return INTERPRET_OK;
                }
                vm.stackTop = frame->slots;
                push(result);
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_PRINT: {
                if (size()) {
                    printValue(pop());
                    printf("\n");
                }
                break;
            }
            case OP_CALL: {
                uint8_t argCount = READ_BYTE();
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                frame = &vm.frames[vm.frameCount - 1];
                break;
            }
            case OP_SUBSCRIPT: {
                Value key = pop();
                if (!subscript(key)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SWAP: {
                if (size()) {
                    Value a = pop();
                    Value b = pop();
                    push(a);
                    push(b);
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
                    ERR_PRINT("Did you mean one of: ");
                    hashmap_iter(&vm.globals, hashmap_err_print_key, NULL);
                    ERR_PRINT("\n");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_SET_LOCAL_LONG:
            case OP_SET_LOCAL: {
                int slot = (instruction == OP_SET_LOCAL) ? READ_BYTE() : READ_24BITS();
                frame->slots[slot] = peek(0);
                break;
            }
            case OP_GET_LOCAL_LONG:
            case OP_GET_LOCAL: {
                int slot = (instruction == OP_GET_LOCAL) ? READ_BYTE() : READ_24BITS();
                push(frame->slots[slot]);
                break;
            }
            case OP_EQUAL: {
                push(BOOL_VAL(valuesEqual(pop(), pop())));
                break;
            }
            case OP_JUMP: {
                int offset = READ_24BITS();
                frame->ip += offset; // wat about negative
                break;
            }
            case OP_NEG_JUMP: {
                int offset = READ_24BITS();
                frame->ip -= offset; // wat about negative
                break;
            }
            case OP_JUMP_IF_FALSE: {
                int offset = READ_24BITS();
                if (isFalsey(peek(0))) {
                    frame->ip += offset; // wat about negative
                }
                break;
            }
            case OP_INIT_ARRAY: {
                ObjArray* array = allocateArray(16);
                push(OBJ_VAL(array));
                break;
            }
            case OP_INSERT_ARRAY: {
                Value value = pop();
                ObjArray* array = AS_ARRAY(peek(0));
                insertArray(array, array->length, value);
                break;
            }
            case OP_INIT_HASHMAP: {
                ObjHashmap* hm = allocateHashmap(8);
                push(OBJ_VAL(hm));
                break;
            }
            case OP_INSERT_HASHMAP: {
                Value value = pop();
                Value key = pop();
                ObjHashmap* hm = AS_HASHMAP(peek(0));
                hashmap_add(&hm->map, key, value);
                break;
            }
            case OP_CONSTANT: push(READ_CONSTANT()); break;
            case OP_CONSTANT_LONG: push(READ_CONSTANT_LONG()); break;
            case OP_NOT: push(BOOL_VAL(isFalsey(pop()))); break;
            case OP_BITNEG: push(INTEGER_VAL(~pop_int())); break;
            case OP_SIZE: if (IS_STRING(peek(0))) {
                push(INTEGER_VAL(strlen(AS_CSTRING(pop()))));
            } else if (IS_ARRAY(peek(0))) {
                push(INTEGER_VAL(ARRAY_LENGTH(pop())));
            } else if (IS_HASHMAP(peek(0))) {
                push(INTEGER_VAL(HASHMAP_LENGTH(pop())));
            } else {
                push(INTEGER_VAL(sizeof(Value)));
            }
            break;
            case OP_GREATER: {
                Value b = pop();
                Value a = pop();
                push(IS_DOUBLE(a) || IS_DOUBLE(b) ? \
                    BOOL_VAL(AS_DOUBLE(a) > AS_DOUBLE(b)) : \
                    BOOL_VAL(AS_INTEGER(a) > AS_INTEGER(b))); \
                break;
            }
            case OP_LESS: {
                Value b = pop();
                Value a = pop();
                push(IS_DOUBLE(a) || IS_DOUBLE(b) ? \
                    BOOL_VAL(AS_DOUBLE(a) < AS_DOUBLE(b)) : \
                    BOOL_VAL(AS_INTEGER(a) < AS_INTEGER(b))); \
                break;
            }
            case OP_ADD: {
                if (IS_STRING(peek(0)) || IS_STRING(peek(1))) {
                    if (!(IS_STRING(peek(0)) && IS_STRING(peek(1)))) {
                        runtimeError("Strings can only be added to other strings");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    // TODO convert stuff to string
                    // TODO string interpolation whooo, fast string builder?
                    concatenate();
                } else if (IS_ARRAY(peek(0)) || IS_ARRAY(peek(1))) {
                    if (!(IS_ARRAY(peek(0)) && IS_ARRAY(peek(1)))) {
                        runtimeError("Arrays can only be added to other arrays");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    concatenateArrays();
                } else {
                    ARITH_BIN_OP(+);
                }
                break;
            }
            case OP_NEG: push(INTEGER_VAL(-1)); ARITH_BIN_OP(*); break;
            case OP_SUB: ARITH_BIN_OP(-); break;
            case OP_MUL: ARITH_BIN_OP(*); break;
            case OP_DIV: {
                if (IS_ZERO(peek(0))) {
                    runtimeError("Ignoring division by zero! Returning infinity.");
                    pop();
                    pop();
                    push(DOUBLE_VAL(INFINITY));
                    break;
                }
                ARITH_BIN_OP(/);
                break;
            }
            case OP_BITAND: INT_BIN_OP(&); break;
            case OP_BITOR: INT_BIN_OP(|); break;
            case OP_BITXOR: INT_BIN_OP(^); break;
            case OP_LEFT_SHIFT: INT_BIN_OP(<<); break;
            // TODO arithmetic shift >>>
            case OP_RIGHT_SHIFT: {
                unsigned b = (unsigned)pop_int();
                unsigned a = (unsigned)pop_int();
                push(INTEGER_VAL(a >> b));
                break;
            }
            case OP_REMAINDER: DOUBLE_BIN_OP(fmod); break;
            case OP_EXP: DOUBLE_BIN_OP(pow); break;
            case OP_NIL: push(NIL_VAL); break;
            case OP_FALSE: push(BOOL_VAL(false)); break;
            case OP_TRUE: push(BOOL_VAL(true)); break;
            case OP_INF: push(DOUBLE_VAL(INFINITY)); break;
            case OP_NAN: push(DOUBLE_VAL(NAN)); break;
        }
    }
}

// This function takes ownership of chunk and will call freeChunk!
InterpretResult interpretChunk(Chunk* chunk) {
    initVM();
    ObjString* name = copyString("interpretChunk", sizeof("interpretChunk"));
    ObjFunction* func = newFunction(name, chunk);
    push(OBJ_VAL(func));
    call(func, 0);
    InterpretResult result = run();
    freeVM();
    return result;
}

InterpretResult interpretOrPrint(const char* string, bool printOnly) {
    initVM();
    ObjFunction* func = compile(string);
    if (!func) {
        return INTERPRET_COMPILE_ERROR;
    }
    if (printOnly) {
        disChunk(&func->chunk, "compileAndPrint");
        freeVM();
        return INTERPRET_OK;
    }
    push(OBJ_VAL(func));
    call(func, 0);
    InterpretResult result = run();
    freeVM();
    return result;
}

InterpretResult interpret(const char* string) {
    return interpretOrPrint(string, false);
}
