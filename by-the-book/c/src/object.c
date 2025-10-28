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

// takeString is allocateString
ObjString* allocateString(char* chars, size_t length, size_t hash) {
    ObjString* interned = hashmap_get_str(&vm.strings, chars, length, hash);
    if (interned) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }
    ObjString* string = (ObjString*)allocateObj(sizeof(ObjString) + sizeof(char) * (length + 1), OBJ_STRING);
    string->length = length;
    string->hash = hash;
    memcpy(string->chars, chars, length);
    FREE_ARRAY(char, chars, length + 1); // TODO inefficient ugh
    // Turn the chars into a C-string
    string->chars[length] = '\0';
    hashmap_add(&vm.strings, OBJ_VAL(string), NIL_VAL);
    return string;
}
    
ObjString* copyString(const char* chars, size_t length) {
    size_t hash = hashString(chars, length);
    ObjString* interned = hashmap_get_str(&vm.strings, chars, length, hash);
    if (interned) {
        return interned;
    }
    ObjString* string = (ObjString*)allocateObj(sizeof(ObjString) + sizeof(char) * (length + 1), OBJ_STRING);
    string->length = length;
    string->hash = hash;
    memcpy(string->chars, chars, length);
    // Turn the chars into a C-string
    string->chars[length] = '\0';
    hashmap_add(&vm.strings, OBJ_VAL(string), NIL_VAL);
    return string;
}

void freeObject(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)obj;
            free(string);
            break;
        }
    }
}

