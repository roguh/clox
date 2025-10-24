#ifndef clox_value_h
#define clox_value_h

#include "common.h"

typedef double Value;

typedef struct {
    int capacity;
    int count;
    Value* values;
} ValueArray;

typedef ValueArray Values;

void initValues(Values* values);
void freeValues(Values* values);
void writeValues(Values* values, Value value);
void printValue(Value value);

#endif
