#ifndef ROGUH_HASHMAP
#define ROGUH_HASHMAP
#define ROGUH_OA_HASHMAP

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "object.h"
#include "value.h"

size_t hashString(const char* chars, size_t length);
size_t hashInt(unsigned int elem);
size_t hashAny(Value val);

#ifndef HASHMAP_KEY_TYPE
#define HASHMAP_KEY_TYPE Value
#define HASHMAP_VALUE_TYPE Value
// Compare two keys
#define HASHMAP_EQUAL(a, b) (valuesEqual(a, b))
#endif

#define hashmap_debug(...) fprintf(stderr, __VA_ARGS__)

typedef struct hashmap_item {
    HASHMAP_KEY_TYPE key;
    HASHMAP_VALUE_TYPE value;
    bool empty;
} hashmap_item;

typedef size_t (*hash_function)(HASHMAP_KEY_TYPE key);

typedef enum {
    QUADRATIC,
    LINEAR,
    // idk about this one
    // HASH_HASH,
} hashmap_scheme_t;

typedef struct hashmap_t {
    hashmap_item* entries;
    hash_function hash;
    size_t total;
    size_t capacity;
    size_t max_collisions;
    hashmap_scheme_t open_addressing_scheme;
} hashmap_t;

typedef void (*hashmap_iterator)(hashmap_t* map, size_t index, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value, void* data);

void hashmap_init(hashmap_t* map, size_t capacity, hash_function hasher);
void hashmap_free(hashmap_t* map);
size_t hashmap_len(hashmap_t* map);
HASHMAP_VALUE_TYPE hashmap_get(hashmap_t* map, HASHMAP_KEY_TYPE key, bool* not_found);
ObjString* hashmap_get_str(hashmap_t* map, const char* chars, size_t length, size_t hash);
bool hashmap_add(hashmap_t* map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value);
bool hashmap_set(hashmap_t* map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value);
bool hashmap_remove(hashmap_t* map, HASHMAP_KEY_TYPE key);
bool hashmap_iter(hashmap_t* map, hashmap_iterator func, void* data);

#endif
