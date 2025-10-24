#include <stdio.h>

#include "../chunk.h"
#include "../debug.h"
#include "../vm.h"

void testChunk1() {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN, 123);
    disChunk(&chunk, "test return");
    freeChunk(&chunk);
}

void testChunk2() {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, addConstant(&chunk, 3.14159265), 123);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, addConstant(&chunk, 2 * 3.14159265), 123);
    writeChunk(&chunk, OP_CONSTANT, 123);
    writeChunk(&chunk, addConstant(&chunk, 3 * 3.14159265), 123);
    writeChunk(&chunk, OP_RETURN, 123);
    disChunk(&chunk, "test return constant");
    freeChunk(&chunk);
}

void testChunk3() {
    Chunk chunk;
    initChunk(&chunk);
    for (int i = 0; i < MIN_SIZE_TO_CONSTANT_LONG + 1; i++) {
        writeConstant(&chunk, i + 3.14159265, 123);
    }
    writeChunk(&chunk, OP_RETURN, 123);
    disChunk(&chunk, "test return many constants (expect 2 OP_CONSTANT_LONG instructions)");
    freeChunk(&chunk);
}

void testRun1() {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN, 123);
    initVM();
    int result = interpret(&chunk);
    freeVM();
    printf("execution result: %d\n", result);
    freeChunk(&chunk);
}

void testRun2() {
    Chunk chunk;
    initChunk(&chunk);
    writeConstant(&chunk, 3.14159265, 122);
    writeChunk(&chunk, OP_NEG, 123);
    writeChunk(&chunk, OP_RETURN, 123);

    initVM();
    int result = interpret(&chunk);
    freeVM();
    printf("execution result: %d\n", result);
    freeChunk(&chunk);
}

void testAll() {
    testChunk1();
    testChunk2();
    testChunk3();
    testRun1();
    testRun2();
}
