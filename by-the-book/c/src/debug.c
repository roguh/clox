#include <stdio.h>

#include "debug.h"
#include "chunk.h"
#include "value.h"

void disChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = disInstruction(chunk, offset);
    }
}

static int constantByteInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t slot = chunk->code[offset + 1];
    printf("%-16s %4d\n", name, slot);
    return offset + 2;
}

static int constantLongByteInstruction(const char* name, Chunk* chunk, int offset) {
    int slot = chunk->code[offset + 1] | chunk->code[offset + 2] << 8 | chunk->code[offset + 3] << 16;
    printf("%-16s %4d\n", name, slot);
    return offset + 4;
}

static int jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    int jump = chunk->code[offset + 1] | chunk->code[offset + 2] << 8 | chunk->code[offset + 3] << 16;
    printf("%-16s %4d -> %d\n", name, offset, offset + SIZE_OF_24BIT_ARGS + 1 + sign * jump);
    return offset + 4;
}

static int constantInstruction(const char* name, Chunk* chunk, int offset) {
    uint8_t addr = chunk->code[offset + 1];
    printf("%-16s %4d '", name, addr);
    printValue(chunk->constants.values[addr]);
    printf("'\n");
    return offset + 2;
}

static int constantLongInstruction(const char* name, Chunk* chunk, int offset) {
    int addr = chunk->code[offset + 1] | chunk->code[offset + 2] << 8 | chunk->code[offset + 3] << 16;
    printf("%-16s %4d '", name, addr);
    printValue(chunk->constants.values[addr]);
    printf("'\n");
    return offset + 4;
}

static int simpleInstruction(const char* name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

int disInstruction(Chunk* chunk, int offset) {
    printf("%04d ", offset);
    if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
        printf("   | ");
    } else {
        printf("% 4d ", chunk->lines[offset]);
    }
    OpCode instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_DEFINE_GLOBAL:
            return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
        case OP_DEFINE_GLOBAL_LONG:
            return constantLongInstruction("OP_DEFINE_GLOBAL_LONG", chunk, offset);
        case OP_GET_GLOBAL:
            return constantInstruction("OP_GET_GLOBAL", chunk, offset);
        case OP_GET_GLOBAL_LONG:
            return constantLongInstruction("OP_GET_GLOBAL_LONG", chunk, offset);
        case OP_SET_GLOBAL:
            return constantInstruction("OP_SET_GLOBAL", chunk, offset);
        case OP_SET_GLOBAL_LONG:
            return constantLongInstruction("OP_SET_GLOBAL_LONG", chunk, offset);
        case OP_GET_LOCAL:
            return constantByteInstruction("OP_GET_LOCAL", chunk, offset);
        case OP_GET_LOCAL_LONG:
            return constantLongByteInstruction("OP_GET_LOCAL_LONG", chunk, offset);
        case OP_SET_LOCAL:
            return constantByteInstruction("OP_SET_LOCAL", chunk, offset);
        case OP_SET_LOCAL_LONG:
            return constantLongByteInstruction("OP_SET_LOCAL_LONG", chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_NEG_JUMP:
            return jumpInstruction("OP_NEG_JUMP", -1, chunk, offset);
        case OP_NEG:
            return simpleInstruction("OP_NEG", offset);
        case OP_NOT:
            return simpleInstruction("OP_NOT", offset);
        case OP_EQUAL:
            return simpleInstruction("OP_EQUAL", offset);
        case OP_GREATER:
            return simpleInstruction("OP_GREATER", offset);
        case OP_LESS:
            return simpleInstruction("OP_LESS", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUB:
            return simpleInstruction("OP_SUB", offset);
        case OP_SIZE:
            return simpleInstruction("OP_SIZE", offset);
        case OP_BITAND:
            return simpleInstruction("OP_BITAND", offset);
        case OP_BITOR:
            return simpleInstruction("OP_BITOR", offset);
        case OP_BITXOR:
            return simpleInstruction("OP_BITXOR", offset);
        case OP_BITNEG:
            return simpleInstruction("OP_BITNEG", offset);
        case OP_MUL:
            return simpleInstruction("OP_MUL", offset);
        case OP_DIV:
            return simpleInstruction("OP_DIV", offset);
        case OP_REMAINDER:
            return simpleInstruction("OP_REMAINDER", offset);
        case OP_EXP:
            return simpleInstruction("OP_EXP", offset);
        case OP_LEFT_SHIFT:
            return simpleInstruction("OP_LEFT_SHIFT", offset);
        case OP_RIGHT_SHIFT:
            return simpleInstruction("OP_RIGHT_SHIFT", offset);
        case OP_PRINT:
            return simpleInstruction("OP_PRINT", offset);
        case OP_POP:
            return simpleInstruction("OP_POP", offset);
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        case OP_INIT_ARRAY:
            return simpleInstruction("OP_INIT_ARRAY", offset);
        case OP_INSERT_ARRAY:
            return simpleInstruction("OP_INSERT_ARRAY", offset);
        case OP_INIT_HASHMAP:
            return simpleInstruction("OP_INIT_HASHMAP", offset);
        case OP_INSERT_HASHMAP:
            return simpleInstruction("OP_INSERT_HASHMAP", offset);
        case OP_SUBSCRIPT:
            return simpleInstruction("OP_SUBSCRIPT", offset);
        case OP_INVALID:
            return simpleInstruction("OP_INVALID", offset);
    }
    printf("unknown opcode %d\n", instruction);
    return offset + 1;
}
