#include "common.h"
#include "hashmap.h"
#include "value.h"
#include "object.h"

size_t hashString(const char* chars, size_t length) {
    size_t hash = 2166136261u;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)chars[i];
        hash *= 16777619;
    }
    return hash;
}

size_t hashInt(unsigned int elem) {
  size_t c2=0x27d4eb2d; // a prime or an odd constant
  elem = (elem ^ 61) ^ (elem >> 16);
  elem = elem + (elem << 3);
  elem = elem ^ (elem >> 4);
  elem = elem * c2;
  elem = elem ^ (elem >> 15);
  return elem;
}

size_t hashAny(Value val) {
    switch (val.type) {
        case VAL_NIL: return hashInt(0);
        case VAL_DOUBLE: return hashInt(AS_INTEGER(val));
        case VAL_INT: return hashInt(AS_INTEGER(val));
        case VAL_BOOL: return hashInt(AS_INTEGER(val));
        case VAL_OBJ: return hashString(AS_CSTRING(val), STRING_LENGTH(val));
    }
    return (size_t)0; // unreachable thanks to -Werror=switch
}

/**
 * Create a new hashmap with the given capacity and hash function.
 * The hash function depends on the contents of the hashmap's keys.
 *
 * ESSENTIAL!
 */
void hashmap_init(hashmap_t* map, size_t capacity, hash_function hasher) {
    // hashmap_t* map = (hashmap_t*)calloc(1, sizeof(hashmap_t));

    // Ensure the capacity is a power of 2
    if ((capacity == 0) || !((capacity & (capacity - 1)) == 0)) {
        size_t new_capacity = 1;
        while (new_capacity < capacity) {
            new_capacity <<= 1;
        }
        hashmap_debug("DEBUG: Rounded capacity up to power of 2, %zu -> %zu\n", capacity, new_capacity);
        capacity = new_capacity;
    }

    map->hash = hasher;
    map->total = 0;
    map->capacity = capacity;
    map->entries = (hashmap_item*)calloc(capacity, sizeof(hashmap_item));
    map->max_collisions = capacity < 16 ? capacity : 16; // jeez rick
    map->open_addressing_scheme = QUADRATIC;
    for (int i = 0; i < map->capacity; i++) {
        map->entries[i].empty = true;
    }
}

/**
 * Return the size of this hashmap.
 */
size_t hashmap_len(hashmap_t* map) {
    return map->total;
}

void _hashmap_free_entries(hashmap_t* map) {
    // Might need to free items, e.g. if keys/values are malloc'd
    free(map->entries);
    map->entries = NULL;
}

/**
 * Free all the memory of a hashmap.
 * Will not free element of a key or value, use hashmap_iter to free those if they are on the heap.
 */
void hashmap_free(hashmap_t* map) {
    _hashmap_free_entries(map);
}

/**
 * Return the ideal index of a given key in this hashmap.
 * Guaranteed to return a number between 0 and map->capacity-1
 *
 * ESSENTIAL!
 */
size_t _hashmap_index(hashmap_t* map, HASHMAP_KEY_TYPE key) {
    // mod is slow, ensure capacity is power of 2 and use a bitmask
    // Make sure this matches _hashmap_index
    return map->hash(key) & (map->capacity - 1);
}

/**
 * Call the given function on all the hashmap's key-value pairs.
 *
 * Returns false if the hashmap is empty.
 */
bool hashmap_iter(hashmap_t* map, hashmap_iterator func, void* data) {
    size_t index = 0;
    for (int i = 0; i < map->capacity; i++) {
        hashmap_item entry = map->entries[i];
        if (!entry.empty) {
            func(map, index, entry.key, entry.value, data);
            index++;
        }
    }
    return index > 0;
}

/**
 * Internal function to get the empty slot where a key belongs OR the slot where it exists.
 * All the open addressing logic is contained within this function!
 * ESSENTIAL!
 */
hashmap_item* _hashmap_get(hashmap_t* map, HASHMAP_KEY_TYPE key, size_t* _collisions) {
    size_t index = _hashmap_index(map, key);
    size_t collisions = 0;
    while (collisions < map->max_collisions) {
        hashmap_item* entry = &map->entries[index];
        // If the item contains the key and is not empty, return it
        if (HASHMAP_EQUAL(entry->key, key) 
                || entry->empty) {
            if (_collisions) {
                *_collisions = collisions;
            }
            return entry;
        }
        switch (map->open_addressing_scheme) {
        case (QUADRATIC):
            index = index * index + 1;
            break;
        case (LINEAR):
        default:
            // Classic!
            index = index + 1;
            break;
        }
        // Bounds check? 0<=index<cap
        index = index & (map->capacity - 1);
        collisions += 1;
    }
    // If none found, return null
    if (_collisions) {
        *_collisions = collisions;
    }
    return NULL;
}

ObjString* hashmap_get_str(hashmap_t* map, const char* chars, size_t length, size_t hash) {
    size_t index = hash & (map->capacity - 1); // Make sure this matches _hashmap_index
    size_t collisions = 0;
    while (collisions < map->max_collisions) {
        hashmap_item entry = map->entries[index];
        // If the item contains the key and is not empty, return it
        if (entry.empty) {
            // implement tombstones later?
            return NULL;
        } else {
            ObjString* str_key = AS_STRING(entry.key);
            if (str_key->length == length 
             && str_key->hash == hash
             && memcmp(str_key->chars, chars, length) == 0) {
                return str_key;
            }
        }
        switch (map->open_addressing_scheme) {
        case (QUADRATIC):
            index = index * index + 1;
            break;
        case (LINEAR):
        default:
            // Classic!
            index = index + 1;
            break;
        }
        // Bounds check? 0<=index<cap
        index = index & (map->capacity - 1);
        collisions += 1;
    }
    return NULL;
}

/**
 * Return -1 if not found, otherwise return collision count (keys with same hash value)
 *
 * Mostly used for debugging and profiling.
 */
int hashmap_collision_count(hashmap_t* map, HASHMAP_KEY_TYPE key) {
    size_t collisions = 0;
    hashmap_item* entry = _hashmap_get(map, key, &collisions);
    if (entry && !entry->empty) {
        return (int)collisions;
    }
    return -1;
}

/**
 * Add an element to this hashmap, if possible.
 *
 * Returns false if adding this key failed. Could be bad!
 *
 * ESSENTIAL!
 */
bool hashmap_add_without_grow(hashmap_t* map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value) {
    // If it already exists, return false
    hashmap_item* entry = _hashmap_get(map, key, NULL);
    if (!entry || !entry->empty) {
        return false;
    }
    // If missing, add it to the start of the linked list at its index
    entry->key = key;
    entry->value = value;
    entry->empty = false;
    map->total++;
    return true;
}


/**
 * Iterator to copy a key-value pair to a newly allocated hashmap.
 *
 * OPTIONAL. Only needed if the map's capacity will need to grow.
 */
void _move_to_new(hashmap_t* old, size_t index, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value, void* data) {
    // Add each value in the old map to the new map
    hashmap_t* newmap = (hashmap_t*)data;
    if (!hashmap_add_without_grow(newmap, key, value)) {
        // What to do if this add fails!?
        hashmap_debug("WARNING! UNABLE TO ADD KEY %zu\n", index);
    }
}

/**
 * Increase the map's capacity by the given factor (example: 2 or 4)
 *
 * OPTIONAL. Only needed if the map's capacity will need to grow.
 */
void hashmap_grow(hashmap_t* map, size_t factor) {
    if (factor < 2 || factor > 10) {
        factor = 2;
    }
    // Make sure all parameters are the same, except capacity
    hashmap_t newmap;
    hashmap_init(&newmap, map->capacity * factor, map->hash);
    newmap.max_collisions = map->max_collisions;
    newmap.open_addressing_scheme = map->open_addressing_scheme;

    hashmap_iter(map, _move_to_new, (void*)&newmap);
    if (newmap.total != map->total) {
        hashmap_debug("WARNING! RESIZE FAILURE? newmap size=%zu old size=%zu\n", newmap.total, map->total);
    }
    // Replace map->entries with the newmap map's entries
    _hashmap_free_entries(map);
    map->capacity = map->capacity * factor;
    map->entries = newmap.entries;
}

/**
 * Add an element to this hashmap, if possible.
 *
 * Returns false if adding this key failed. Could be bad!
 *
 * OPTIONAL. Only needed if the map's capacity will need to grow.
 */
bool hashmap_add(hashmap_t* map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value) {
    // Resize if needed....
    if (map->total * 2 > map->capacity) {
        hashmap_grow(map, 2);
        hashmap_debug("DEBUG: Growing capacity up to %zu\n", map->capacity);
    }
    return hashmap_add_without_grow(map, key, value);
}

/**
 * Get a key's value.
 *
 * Set bool* to true if the key was not found, otherwise gets the key's value.
 */
HASHMAP_VALUE_TYPE hashmap_get(hashmap_t* map, HASHMAP_KEY_TYPE key, bool* not_found) {
    hashmap_item* entry = _hashmap_get(map, key, NULL);
    if (entry && !entry->empty) {
        if (not_found) {
            *not_found = false;
        }
        return entry->value;
    }
    if (not_found) {
        *not_found = true;
    }
    return NIL_VAL;
}

/**
 * Return false if key is not found, true if key was changed.
 */
bool hashmap_set(hashmap_t* map, HASHMAP_KEY_TYPE key, HASHMAP_VALUE_TYPE value) {
    hashmap_item* entry = _hashmap_get(map, key, NULL);
    if (entry && !entry->empty) {
        entry->value = value;
        return true;
    }
    return false;
}

/**
 * Attempt to remove a key.
 * Return false if key is not found.
 */
bool hashmap_remove(hashmap_t* map, HASHMAP_KEY_TYPE key) {
    hashmap_item* entry = _hashmap_get(map, key, NULL);
    if (!entry || entry->empty) {
        return false;
    }
    entry->empty = true;
    map->total--;
    return true;
}

