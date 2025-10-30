#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define UINT8_COUNT (UINT8_MAX + 1)

extern bool DEBUG_TRACE;

#define ERR_PRINT(...) fprintf(stderr, ##__VA_ARGS__)

#endif
