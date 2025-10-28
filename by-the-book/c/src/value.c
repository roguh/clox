#include <stdio.h>

#include "value.h"
#include "memory.h"
#include "object.h"

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

static const char* objectType(ObjType ty) {
    switch (ty) {
        case OBJ_STRING: return "OBJ_STRING";
    }
    return "unknown";
}

void printValue(Value value) {
    switch (value.type) {
        // Exhaustive!
        case VAL_NIL: printf("nil"); break;
        case VAL_DOUBLE: printf("%.16lg", AS_DOUBLE(value)); break;
        case VAL_INT: printf("%d", AS_INTEGER(value)); break;
        case VAL_BOOL: printf("%s", AS_BOOL(value) ? "true" : "false"); break;
        case VAL_OBJ: printf("%s", AS_CSTRING(value)); break;
    }
}

double AS_DOUBLE(Value value) {
    if (IS_DOUBLE(value)) {
        return value.as._double;
    } else {
        return (double)value.as._int;
    }
}

int AS_INTEGER(Value value) {
    if (IS_INTEGER(value) || IS_BOOL(value)) {
        return value.as._int;
    } else {
        return (double)value.as._double;
    }
}

bool valuesEqual(Value a, Value b) {
    if (a.type != b.type) {
        return false;
    }
    switch (a.type) {
        case VAL_NIL: return a.type == b.type;
        case VAL_INT: return AS_INTEGER(a) == AS_INTEGER(b);
        case VAL_DOUBLE: return AS_DOUBLE(a) == AS_DOUBLE(b);
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_OBJ: {
            ObjString* aS = AS_STRING(a);
            ObjString* bS = AS_STRING(b);
            return aS->length == bS->length && memcmp(aS->chars, bS->chars, aS->length) == 0;
        }
    }
    return false;
}

