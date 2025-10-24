#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

void disChunk(Chunk* chunk, const char* name);
int disInstruction(Chunk* chunk, int offset);

#endif
