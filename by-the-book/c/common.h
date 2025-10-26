#ifndef clox_common_h
#define clox_common_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern bool DEBUG_TRACE;

#define ERR_PRINT(fmt) fprintf(fmt, ##__VA_ARGS__)

#endif
