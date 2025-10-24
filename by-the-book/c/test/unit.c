#include <stdio.h>

#include "../chunk.h"
#include "../debug.h"

void testChunk() {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN);
    disChunk(&chunk, "test return");
    freeChunk(&chunk);
}

void testChunk2() {
    Chunk chunk;
    initChunk(&chunk);
    int constant = addConstant(&chunk, 3.14159265);
    writeChunk(&chunk, OP_CONSTANT);
    writeChunk(&chunk, constant);
    writeChunk(&chunk, OP_RETURN);
    disChunk(&chunk, "test return constant");
    freeChunk(&chunk);
}

void testAll() {
    testChunk();
    testChunk2();
}
