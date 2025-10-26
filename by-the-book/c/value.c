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
        OBJ_STRING: return "OBJ_STRING";
    }
}

void printValue(Value value) {
    switch (value.type) {
        // Exhaustive!
        case VAL_NIL: printf("nil"); break;
        case VAL_DOUBLE: printf("%lf", AS_DOUBLE(value)); break;
        case VAL_INT: printf("%d", AS_INTEGER(value)); break;
        case VAL_BOOL: printf("%s", AS_BOOL(value) ? "true" : "false"); break;
        case VAL_OBJ: printf("<%s %p>", objectType(AS_OBJ(value)->type), (void*)AS_OBJ(value)); break;
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
