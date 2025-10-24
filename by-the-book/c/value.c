#include <stdio.h>
#include <stdlib.h>

#include "value.h"
#include "memory.h"

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
    values->values[values->count++] = value;
}

void printValue(Value value) {
    printf("%.15lf", value);
}
