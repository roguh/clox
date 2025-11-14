#include <time.h>
#include <math.h>

#include "value.h"
#include "object.h"
#include "vm.h"
#include "hashmap.h"

static Value FFI_clock(int argCount, Value* args) {
    return DOUBLE_VAL((double)clock() / CLOCKS_PER_SEC);
}

static Value FFI_line(int argCount, Value* args) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    int line = frame->function->chunk.lines[frame->ip - frame->function->chunk.code - 1];
    return INTEGER_VAL(line);
}

static Value FFI_col(int argCount, Value* args) {
    CallFrame* frame = &vm.frames[vm.frameCount - 1];
    int col = frame->function->chunk.columns[frame->ip - frame->function->chunk.code - 1];
    return INTEGER_VAL(col);
}

static Value FFI_globals(int argCount, Value* args) {
    return OBJ_VAL(vm.globals);
}


static void _FFI_keys_add_item(hashmap_t* hm, size_t ix, Value key, Value val, void* data) {
    ObjArray* keys = AS_ARRAY(getArray((ObjArray*)data, 0));
    ObjArray* vals = AS_ARRAY(getArray((ObjArray*)data, 1));
    insertArray(keys, ix, key);
    insertArray(vals, ix, val);
}

static Value FFI_keys_and_values(int argCount, Value* args) {
    Value arg = args[0];
    if (IS_HASHMAP(arg)) {
        hashmap_t hm = AS_HASHMAP(arg)->map;
        ObjArray* keys = allocateArray(hashmap_len(&hm));
        ObjArray* vals = allocateArray(hashmap_len(&hm));
        ObjArray* result = allocateArray(8);
        insertArray(result, 0, OBJ_VAL(keys));
        insertArray(result, 1, OBJ_VAL(vals));
        hashmap_iter(&hm, _FFI_keys_add_item, result);
        return OBJ_VAL(result);
    } else {
        return NIL_VAL;
    }
}

static Value FFI_keys(int argCount, Value* args) {
    Value ret = FFI_keys_and_values(argCount, args);
    if (IS_ARRAY(ret) && AS_ARRAY(ret)->length == 2) {
        return getArray(AS_ARRAY(ret), 0);
    }
    return ret;
}

static Value FFI_values(int argCount, Value* args) {
    Value ret = FFI_keys_and_values(argCount, args);
    if (IS_ARRAY(ret) && AS_ARRAY(ret)->length == 2) {
        return getArray(AS_ARRAY(ret), 1);
    }
    return ret;
}

#define NUMERIC_ASSERT(func) \
    if (argCount == 0) { \
        runtimeError("%s expects at least 1 argument", func); \
        return NIL_VAL; \
    } \
    for (int i = 0; i < argCount; i++) { \
        if (IS_OBJ(args[i])) { \
            runtimeError("%s expects a number", func); \
            return NIL_VAL; \
        } \
    }

#define DOUBLE_DOUBLE(func) \
    static Value FFI_##func(int argCount, Value* args) { \
        NUMERIC_ASSERT(#func); \
        return DOUBLE_VAL(func(AS_DOUBLE(args[0]))); \
    }

#define DOUBLE_DOUBLE_2(func) \
    static Value FFI_##func(int argCount, Value* args) { \
        NUMERIC_ASSERT(#func); \
        return DOUBLE_VAL(func(AS_DOUBLE(args[0]), AS_DOUBLE(args[1]))); \
    }

#define DOUBLE_DOUBLE_3(func) \
    static Value FFI_##func(int argCount, Value* args) { \
        NUMERIC_ASSERT(#func); \
        return DOUBLE_VAL(func( \
            AS_DOUBLE(args[0]), AS_DOUBLE(args[1]), AS_DOUBLE(args[2]) \
        )); \
    }

// Define mathematical functions that take a double and return a double
/// Exponential functions
DOUBLE_DOUBLE(exp)
DOUBLE_DOUBLE(exp2)
DOUBLE_DOUBLE(expm1)
DOUBLE_DOUBLE(log)
DOUBLE_DOUBLE(log10)
DOUBLE_DOUBLE(log2)
DOUBLE_DOUBLE(log1p)

/// Basic operations
DOUBLE_DOUBLE(fabs)
DOUBLE_DOUBLE_2(fmod)
DOUBLE_DOUBLE_2(remainder)
DOUBLE_DOUBLE_3(fma)
DOUBLE_DOUBLE_2(fmax)
DOUBLE_DOUBLE_2(fmin)
DOUBLE_DOUBLE_2(fdim)

/// Power functions
DOUBLE_DOUBLE_2(pow)
DOUBLE_DOUBLE(sqrt)
DOUBLE_DOUBLE(cbrt)
DOUBLE_DOUBLE_2(hypot)

/// Trig functions
DOUBLE_DOUBLE(sin)
DOUBLE_DOUBLE(cos)
DOUBLE_DOUBLE(tan)
DOUBLE_DOUBLE(asin)
DOUBLE_DOUBLE(acos)
DOUBLE_DOUBLE(atan)
DOUBLE_DOUBLE_2(atan2)

/// Hyperbolic functions
DOUBLE_DOUBLE(sinh)
DOUBLE_DOUBLE(cosh)
DOUBLE_DOUBLE(tanh)
DOUBLE_DOUBLE(asinh)
DOUBLE_DOUBLE(acosh)
DOUBLE_DOUBLE(atanh)

/// Error and gamma functions
DOUBLE_DOUBLE(erf)
DOUBLE_DOUBLE(erfc)
DOUBLE_DOUBLE(tgamma)
DOUBLE_DOUBLE(lgamma)

void bindManyNativeFunctions() {
#define DEF(func, count) defineNative(#func, count, FFI_##func)
    
    defineConstant("sizeofValue", INTEGER_VAL(sizeof(Value)));
    defineConstant("sizeofInt", INTEGER_VAL(sizeof(int)));
    defineConstant("sizeofDouble", INTEGER_VAL(sizeof(double)));
    defineConstant("pi", DOUBLE_VAL(3.141592653589793));
    defineConstant("e", DOUBLE_VAL(2.718281828459045));
    defineNative("__line__", 0, FFI_line);
    defineNative("__col__", 0, FFI_col);
    DEF(clock, 0);
    DEF(globals, 0);
    DEF(keys_and_values, 1);
    DEF(keys, 1);
    DEF(values, 1);

    DEF(sqrt, 1);
    DEF(exp, 1);
    DEF(exp2, 1);
    DEF(expm1, 1);
    DEF(log, 1);
    DEF(log10, 1);
    DEF(log2, 1);
    DEF(log1p, 1);

/// Basic operations
    DEF(fabs, 1);
    DEF(fmod, 1);
    DEF(remainder, 1);
    DEF(fma, 1);
    DEF(fmax, 1);
    DEF(fmin, 1);
    DEF(fdim, 1);

/// Power functions
    DEF(pow, 1);
    DEF(sqrt, 1);
    DEF(cbrt, 1);
    DEF(hypot, 1);

/// Trig functions
    DEF(sin, 1);
    DEF(cos, 1);
    DEF(tan, 1);
    DEF(asin, 1);
    DEF(acos, 1);
    DEF(atan, 1);
    DEF(atan2, 1);

/// Hyperbolic functions
    DEF(sinh, 1);
    DEF(cosh, 1);
    DEF(tanh, 1);
    DEF(asinh, 1);
    DEF(acosh, 1);
    DEF(atanh, 1);

/// Error and gamma functions
    DEF(erf, 1);
    DEF(erfc, 1);
    DEF(tgamma, 1);
    DEF(lgamma, 1);
}
