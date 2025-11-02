#include <stdio.h>
#include <complex.h>

#include "print.h"

void printValueExtra(Value value, bool printQuotes);

void printHashmapItem(hashmap_t* map, long unsigned int index, Value key, Value value, void* data) {
    printValueExtra(key, true);
    printf(": ");
    printValueExtra(value, true);
    if (index < hashmap_len(map) - 1) {
        printf(", ");
    }
}

void printString(const char* chars, size_t length, bool printQuotes) {
    if (printQuotes) {
        char qChar = '"';
        bool escapeQuotes = false;
        for (int i = 0; i < length; i++) {
            if (chars[i] == '"') {
                qChar = '\'';
            }
            if (qChar == '\'' && chars[i] == '\'') {
                escapeQuotes = true;
            }
        }
        if (escapeQuotes) {
            printf("\"");
            for (int i = 0; i < length; i++) {
                char c = chars[i];
                if (c == '"') {
                    printf("\\\"");
                } else {
                    printf("%c", c);
                }
            }
            printf("\"");
        } else {
            printf("%c%.*s%c", qChar, (int)length, chars, qChar);
        }
    } else {
        printf("%.*s", (int)length, chars);
    }
}

void printValueExtra(Value value, bool printQuotes) {
    switch (value.type) { // Exhaustive
        case VAL_NEVER: printf("(VAL null or uninitialized?)"); break;
        case VAL_NIL: printf("nil"); break;
        case VAL_DOUBLE: printf("%.16lg", AS_DOUBLE(value)); break;
        case VAL_FCOMPLEX: {
            float complex v = AS_FCOMPLEX(value);
            if (creal(v) == 0.0) {
                printf("%.16lgj", cimag(v));
            } else {
                printf("(%.16lg%+.16lgj)", creal(v), cimag(v));
            }
            break;
        }
        case VAL_INT: printf("%d", AS_INTEGER(value)); break;
        case VAL_BOOL: printf("%s", AS_BOOL(value) ? "true" : "false"); break;
        case VAL_OBJ: {
            switch (AS_OBJ(value)->type) {
                case OBJ_NEVER: printf("(OBJ null or uninitialized?)"); break;
                case OBJ_FUNCTION:
                    printFunction(AS_FUNCTION(value));
                    break;
                case OBJ_NATIVE:
                    printNative(AS_NATIVE(value));
                    break;
                case OBJ_STRING:
                    printString(AS_STRING(value)->chars, AS_STRING(value)->length, printQuotes);
                    break;
                case OBJ_STRING_VIEW:
                    printString(AS_STRING_VIEW(value)->chars, AS_STRING(value)->length, printQuotes);
                    break;
                case OBJ_ARRAY:
                    printf("[");
                    for (int i = 0; i < ARRAY_LENGTH(value); i++) {
                        printValueExtra(AS_ARRAY(value)->values[i], true);
                        if (i < ARRAY_LENGTH(value) - 1) {
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
            }
        }
    }
}

void printValue(Value value) {
    printValueExtra(value, false);
}

void printFunction(ObjFunction* func) {
    printf("<fn %s>", func->name ? func->name->chars : "<no_name>");
}

void printNative(ObjNative* func) {
    printf("<native %s>", func->name ? func->name->chars : "<no_name>");
}

