#include <stdio.h>

#include "common.h"
#include "test/unit.h"

bool DEBUG_TRACE = false;

int main(int argc, char** argv) {
    if (argc == 2) {
        DEBUG_TRACE = true;
        printf("====== DEBUG_TRACE=true\n");
    }
    testAll();
    return 0;
}
