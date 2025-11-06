#include <stdio.h>

#include "value.h"
#include "hashmap.h"
#include "memory.h"
#include "object.h"

const char* valueTypeNames[] = {
    "VAL_NEVER",
    "VAL_NIL",
    "VAL_DOUBLE",
    "VAL_INT",
    "VAL_FCOMPLEX",
    "VAL_BOOL",
    "VAL_OBJ",
};

void initValues(Values* values) {
    values->count = 0;
    values->capacity = 0;
    values->values = NULL;
}

void freeValues(Values* values) {
    FREE_ARRAY(Value, values->values, values->capacity);
    initValues(values);
}

void writeValues(Values* values, Value value) {
    if (values->capacity < values->count + 1) {
        int old = values->capacity;
        values->capacity = GROW_CAPACITY(old);
        values->values = GROW_ARRAY(Value, values->values, old, values->capacity);
    }
    values->values[values->count] = value;
    values->count++;
}

double AS_DOUBLE(Value value) {
    if (IS_DOUBLE(value)) {
        return value.as._double;
    } else if (IS_INTEGER(value) || IS_BOOL(value)) {
        return (double)AS_INTEGER(value);
    } else if (IS_FCOMPLEX(value)) {
        return (double)AS_FCOMPLEX(value);
    } else {
        return value.as._double;
    }
}

int AS_INTEGER(Value value) {
    if (IS_INTEGER(value) || IS_BOOL(value)) {
        return value.as._int;
    } else if (IS_DOUBLE(value)) {
        return (double)value.as._double;
    } else if (IS_FCOMPLEX(value)) {
        return (int)AS_FCOMPLEX(value);
    } else {
        return value.as._int;
    }
}

float complex AS_FCOMPLEX(Value value) {
    if (IS_FCOMPLEX(value)) {
        return value.as._fcomplex;
    } else if (IS_DOUBLE(value)) {
        return (float complex)AS_DOUBLE(value);
    } else if (IS_INTEGER(value) || IS_BOOL(value)) {
        return (float complex)AS_INTEGER(value);
    } else {
        return value.as._fcomplex;
    }
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) {
        // If the types are different, check if both are numbers
        if (!(IS_NUMBER(a) && IS_NUMBER(b))) {
            // Neither is a number
            return false;
        }
    }
    switch (a.type) { // Exhaustive
        case VAL_NEVER: return false;
        case VAL_NIL: return a.type == b.type;
        case VAL_INT: return AS_INTEGER(a) == AS_INTEGER(b);
        case VAL_FCOMPLEX: return AS_FCOMPLEX(a) == AS_FCOMPLEX(b);
        case VAL_DOUBLE: return AS_DOUBLE(a) == AS_DOUBLE(b);
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_OBJ: return objsEqual(AS_OBJ(a), AS_OBJ(b)); // This function also has an exhaustive switch
    }
    return false;
}
