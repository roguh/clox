#include <stdio.h>

#include "common.h"
#include "scanner.h"
#include "vm.h"

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    return true;
}
