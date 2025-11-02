#ifndef clox_value_h
#define clox_value_h

#include <complex.h>
#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjArray ObjArray;
typedef struct ObjHashmap ObjHashmap;

typedef enum {
    VAL_NEVER,  // Sentinel at enum value 0 to detect uninitialized memory
    VAL_NIL,
    VAL_DOUBLE,
    VAL_INT,
    VAL_FCOMPLEX,
    VAL_BOOL,
    VAL_OBJ,
} ValueType;

// Flat! Struct is packed to fill 8 bytes on a 64-bit arch
typedef struct {
    ValueType type;
    union {
        int _int;
        double _double;
        float complex _fcomplex;
        Obj* _obj;
    } as;
} Value;

#define NIL_VAL ((Value){VAL_NIL, {._int = 0}})
#define DOUBLE_VAL(val) ((Value){VAL_DOUBLE, {._double = val}})
#define INTEGER_VAL(val) ((Value){VAL_INT, {._int = val}})
#define FCOMPLEX_VAL(val) ((Value){VAL_FCOMPLEX, {._fcomplex = val}})
#define BOOL_VAL(val) ((Value){VAL_BOOL, {._int = (val) != 0}})
#define OBJ_VAL(pointer) ((Value){VAL_OBJ, {._obj = (Obj*)(pointer)}})

#define AS_BOOL(value) ((bool)(value).as._int)
#define AS_OBJ(value) ((value).as._obj)

#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_DOUBLE(value) ((value).type == VAL_DOUBLE)
#define IS_FCOMPLEX(value) ((value).type == VAL_FCOMPLEX)
#define IS_INTEGER(value) ((value).type == VAL_INT)
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_OBJ(value) ((value).type == VAL_OBJ)

#define IS_ZERO(value) (AS_DOUBLE(value) == 0.0 || AS_FCOMPLEX(value) == 0.0)

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

// increment this in the "real world"
#define MIN_SIZE_TO_CONSTANT_LONG 8

typedef ValueArray Values;

void initValues(Values* values);
void freeValues(Values* values);
void writeValues(Values* values, Value value);

double AS_DOUBLE(Value value);
int AS_INTEGER(Value value);
float complex AS_FCOMPLEX(Value value);

bool valuesEqual(Value a, Value b);

#endif
