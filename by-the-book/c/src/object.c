#include <stdio.h>
#include <string.h>

#include "hashmap.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include "vm.h"

static Obj* allocateObj(size_t size, ObjType type) {
    Obj* object = (Obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = vm.objects;
    vm.objects = object;
    return object;
}

#define ALLOCATE_OBJ(type, objectType) \
    (type*)allocateObj(sizeof(type), objectType)

// takeString is allocateString
ObjString* allocateString(char* chars, size_t length, size_t hash) {
    ObjString* interned = hashmap_get_str(&vm.strings, chars, length, hash);
    if (interned) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    ObjString* string = ALLOCATE_OBJ(ObjString, OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    hashmap_add(&vm.strings, OBJ_VAL(string), NIL_VAL);
    return string;
}
    
ObjString* copyString(const char* chars, size_t length) {
    size_t hash = hashString(chars, length);
    ObjString* interned = hashmap_get_str(&vm.strings, chars, length, hash);
    if (interned) {
        return interned;
    }
    char* heapChars = ALLOCATE(char, length + 1);
    memcpy(heapChars, chars, length);
    // Turn the chars into a C-string
    heapChars[length] = '\0';
    return allocateString(heapChars, length, hash);
}
