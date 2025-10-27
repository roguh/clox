#include <stdio.h>

#include "../chunk.h"
#include "../debug.h"
#include "../vm.h"

// These only test double values
#define VAL(val) DOUBLE_VAL(val)

void testChunk1(void) {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN, 123, 12);
    disChunk(&chunk, "test return");
    freeChunk(&chunk);
}

void testChunk2(void) {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_CONSTANT, 123, 12);
    writeChunk(&chunk, addConstant(&chunk, VAL(3.14159265)), 123, 12);
    writeChunk(&chunk, OP_CONSTANT, 123, 12);
    writeChunk(&chunk, addConstant(&chunk, VAL(2 * 3.14159265)), 123, 12);
    writeChunk(&chunk, OP_CONSTANT, 123, 12);
    writeChunk(&chunk, addConstant(&chunk, VAL(3 * 3.14159265)), 123, 12);
    writeChunk(&chunk, OP_RETURN, 123, 12);
    disChunk(&chunk, "test return constant");
    freeChunk(&chunk);
}

void testChunk3(void) {
    Chunk chunk;
    initChunk(&chunk);
    for (int i = 0; i < MIN_SIZE_TO_CONSTANT_LONG + 1; i++) {
        writeConstant(&chunk, VAL(i + 3.14159265), 123, 12);
    }
    writeChunk(&chunk, OP_RETURN, 123, 12);
    disChunk(&chunk, "test return many constants (expect 2 OP_CONSTANT_LONG instructions)");
    freeChunk(&chunk);
}

void testRun1(void) {
    Chunk chunk;
    initChunk(&chunk);
    writeChunk(&chunk, OP_RETURN, 123, 12);
    initVM();
    int result = interpretChunk(&chunk);
    freeVM();
    printf("execution result: %d\n", result);
    freeChunk(&chunk);
}

void testRun2(void) {
    Chunk chunk;
    initChunk(&chunk);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeChunk(&chunk, OP_NEG, 123, 12);
    writeChunk(&chunk, OP_RETURN, 123, 12);

    initVM();
    int result = interpretChunk(&chunk);
    freeVM();
    printf("execution result: %d\n", result);
    freeChunk(&chunk);
}

void testRun3(void) {
    Chunk chunk;
    initChunk(&chunk);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeChunk(&chunk, OP_ADD, 123, 12);
    writeChunk(&chunk, OP_PRINT, 123, 12);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeChunk(&chunk, OP_SUB, 123, 12);
    writeChunk(&chunk, OP_PRINT, 123, 12);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeChunk(&chunk, OP_MUL, 123, 12);
    writeChunk(&chunk, OP_PRINT, 123, 12);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeConstant(&chunk, VAL(3.14159265), 122, 11);
    writeChunk(&chunk, OP_DIV, 123, 12);
    writeChunk(&chunk, OP_PRINT, 123, 12);
    writeChunk(&chunk, OP_RETURN, 123, 12);

    initVM();
    int result = interpretChunk(&chunk);
    freeVM();
    printf("execution result: %d\n", result);
    freeChunk(&chunk);
}

void testAll(void) {
    testChunk1();
    testChunk2();
    testChunk3();
    testRun1();
    testRun2();
    testRun3();
}
