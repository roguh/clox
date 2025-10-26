#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    // TODO assert less than 256 ops total
    OP_RETURN,
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_NEG,
    OP_ADD,
    OP_SUB,
    OP_SIZE,
    OP_BITAND,
    OP_BITOR,
    OP_BITXOR,
    OP_BITNEG,
    OP_MUL,
    OP_DIV,
    OP_REMAINDER,
    OP_EXP,
    OP_LEFT_SHIFT,
    OP_RIGHT_SHIFT,
    OP_PRINT,
} OpCode;

typedef struct {
    int count;
    int capacity;
    uint8_t* code;
    int* lines;
    int* columns;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void freeChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line, int column);
void writeConstant(Chunk* chunk, Value value, int line, int column);
int addConstant(Chunk* chunk, Value value);


#endif
