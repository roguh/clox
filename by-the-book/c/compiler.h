#ifndef clox_compiler_h
#define clox_compiler_h

#include "object.h"

bool compile(const char* source, Chunk* chunk, bool debugPrint);

#endif
