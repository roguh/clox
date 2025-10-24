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
