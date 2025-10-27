#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    chunk->columns = NULL;
    initValues(&chunk->constants);
}

void freeChunk(Chunk* chunk) {
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->lines, chunk->capacity);
    FREE_ARRAY(int, chunk->columns, chunk->capacity);
    freeValues(&chunk->constants);
    initChunk(chunk);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line, int column) {
    if (chunk->capacity < chunk->count + 1) {
        int old = chunk->capacity;
        chunk->capacity = GROW_CAPACITY(old);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, old, chunk->capacity);
        chunk->lines = GROW_ARRAY(int, chunk->lines, old, chunk->capacity);
        chunk->columns = GROW_ARRAY(int, chunk->columns, old, chunk->capacity);
    }
    chunk->lines[chunk->count] = line;
    chunk->columns[chunk->count] = column;
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

int addConstant(Chunk* chunk, Value value) {
    writeValues(&chunk->constants, value);
    return chunk->constants.count - 1;
}

void writeConstant(Chunk* chunk, Value value, int line, int column) {
    int offset = addConstant(chunk, value);
    if (chunk->constants.count < MIN_SIZE_TO_CONSTANT_LONG) {
        writeChunk(chunk, OP_CONSTANT, line, column);
        writeChunk(chunk, (uint8_t)offset, line, column);
    } else {
        writeChunk(chunk, OP_CONSTANT_LONG, line, column);
        // not only 1 but 3 bytes!
        writeChunk(chunk, (uint8_t)offset, line, column);
        writeChunk(chunk, (uint8_t)(offset >> 8), line, column);
        writeChunk(chunk, (uint8_t)(offset >> 16), line, column);
    }
}
