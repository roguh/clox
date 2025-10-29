#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include "value.h"

typedef enum {
    // TODO assert less than 256 ops total
    OP_INVALID,
    OP_RETURN,
    OP_PRINT,
    // Stack ops
    OP_POP,
    // Variables
    OP_DEFINE_GLOBAL,
    OP_DEFINE_GLOBAL_LONG,
    OP_GET_GLOBAL,
    OP_GET_GLOBAL_LONG,
    OP_SET_GLOBAL,
    OP_SET_GLOBAL_LONG,
    OP_GET_LOCAL,
    OP_GET_LOCAL_LONG,
    OP_SET_LOCAL,
    OP_SET_LOCAL_LONG,
    // Jumps
    OP_JUMP_IF_FALSE,
    OP_JUMP,
    // Values
    OP_CONSTANT,
    OP_CONSTANT_LONG,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    // Arithmetic
    OP_NEG,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_REMAINDER,
    OP_EXP,
    // Bitwise operatioons
    OP_BITAND,
    OP_BITOR,
    OP_BITXOR,
    OP_BITNEG,
    OP_LEFT_SHIFT,
    OP_RIGHT_SHIFT,
    // Arrays and hashmaps
    OP_SIZE,
    // Boolean operations
    OP_NOT,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
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
int addConstant(Chunk* chunk, Value value);
int writeConstantByOffset(Chunk* chunk, OpCode instr, OpCode instrLong, int offset, int line, int column);
void write24Bit(Chunk* chunk, int offset, int line, int column);
int writeConstant(Chunk* chunk, Value value, int line, int column);

#define SIZE_OF_24BIT_ARGS 3  // 24 bits is 3 bytes


#endif
