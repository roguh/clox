#include <stdio.h>

#include "value.h"
#include "hashmap.h"
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

static void _printValue(Value value, bool printQuotes);

void printHashmapItem(hashmap_t* map, long unsigned int index, Value key, Value value, void* data) {
    _printValue(key, true);
    printf(": ");
    _printValue(value, true);
    if (index < hashmap_len(map) - 1) {
        printf(", ");
    }
}

static void _printValue(Value value, bool printQuotes) {
    switch (value.type) { // Exhaustive
        case VAL_NIL: printf("nil"); break;
        case VAL_DOUBLE: printf("%.16lg", AS_DOUBLE(value)); break;
        case VAL_INT: printf("%d", AS_INTEGER(value)); break;
        case VAL_BOOL: printf("%s", AS_BOOL(value) ? "true" : "false"); break;
        case VAL_OBJ: {
            switch (AS_OBJ(value)->type) {
                case OBJ_STRING:
                    if (printQuotes) {
                        char qChar = '"';
                        bool escapeQuotes = false;
                        for (int i = 0; i < AS_STRING(value)->length; i++) {
                            if (AS_CSTRING(value)[i] == '"') {
                                qChar = '\'';
                            }
                            if (qChar == '\'' && AS_CSTRING(value)[i] == '\'') {
                                escapeQuotes = true;
                            }
                        }
                        if (escapeQuotes) {
                            printf("\"");
                            for (int i = 0; i < AS_STRING(value)->length; i++) {
                                char c = AS_CSTRING(value)[i];
                                if (c == '"') {
                                    printf("\\\"");
                                } else {
                                    printf("%c", c);
                                }
                            }
                            printf("\"");
                        } else {
                            printf("%c%s%c", qChar, AS_CSTRING(value), qChar);
                        }
                    } else {
                        printf("%s", AS_CSTRING(value));
                    }
                    break;
                case OBJ_ARRAY:
                    printf("[");
                    for (int i = 0; i < AS_ARRAY(value)->length; i++) {
                        _printValue(AS_ARRAY(value)->values[i], true);
                        if (i < AS_ARRAY(value)->length - 1) {
                            printf(", ");
                        }
                    }
                    printf("]");
                    break;
                case OBJ_HASHMAP:
                    printf("{");
                    hashmap_iter(&AS_HASHMAP(value)->map, printHashmapItem, NULL);
                    printf("}");
                    break;
                default:
                    printf("<unknown object>");
                    break;
            }
        }
    }
}

void printValue(Value value) {
    _printValue(value, false);
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
    switch (a.type) { // Exhaustive
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
