#ifndef clox_print_h
#define clox_print_h

#include "common.h"
#include "value.h"
#include "object.h"
#include "hashmap.h"

void printValueExtra(Value value, bool printQuotes);
void printValue(Value value);
void printFunction(ObjFunction* func);

#endif
