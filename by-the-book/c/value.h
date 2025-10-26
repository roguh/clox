#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef enum {
    VAL_NIL,
    VAL_DOUBLE,
    VAL_INT,
    VAL_BOOL,
} ValueType;

// Flat! Struct is packed to fill 8 bytes on a 64-bit arch
typedef struct {
    ValueType type;
    union {
        int _int;
        double _double;
    } as;
} Value;

#define NIL_VAL ((Value){VAL_NIL, {._int = 0}})
#define DOUBLE_VAL(val) ((Value){VAL_DOUBLE, {._double = val}})
#define INTEGER_VAL(val) ((Value){VAL_INT, {._int = val}})
#define BOOL_VAL(val) ((Value){VAL_BOOL, {._int = (bool)val}})

#define AS_BOOL(value) ((bool)(value).as._int)

#define IS_NIL(value) ((value).type == VAL_NIL)
#define IS_DOUBLE(value) ((value).type == VAL_DOUBLE)
#define IS_INTEGER(value) ((value).type == VAL_INT)
#define IS_BOOL(value) ((value).type == VAL_BOOL)

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
void printValue(Value value);

double AS_DOUBLE(Value value);
int AS_INTEGER(Value value);

#endif
