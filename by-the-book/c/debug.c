#include <stdio.h>

#include "debug.h"
#include "value.h"

void disChunk(Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = disInstruction(chunk, offset);
    }
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
    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", chunk, offset);
        case OP_CONSTANT_LONG:
            return constantLongInstruction("OP_CONSTANT_LONG", chunk, offset);
        case OP_NEG:
            return simpleInstruction("OP_NEG", offset);
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
        case OP_FALSE:
            return simpleInstruction("OP_FALSE", offset);
        case OP_NIL:
            return simpleInstruction("OP_NIL", offset);
        case OP_TRUE:
            return simpleInstruction("OP_TRUE", offset);
        default:
            printf("unknown opcode %d\n", instruction);
            return offset + 1;
    }
}
