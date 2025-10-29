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


ObjArray* allocateArray(size_t capacity) {
    ObjArray* array = (ObjArray*)allocateObj(sizeof(ObjArray) + sizeof(Value) * capacity, OBJ_ARRAY);
    array->capacity = capacity;
    array->length = 0;
    return array;
}
ObjArray* reallocArray(ObjArray* array, size_t capacity) {
    if (capacity == array->capacity) {
        return array;
    }
    ObjArray* new_array = realloc(array, sizeof(ObjArray) + sizeof(Value) * capacity);
    if (new_array == NULL) {
        fprintf(stderr, "Failed to reallocate array\n");
        exit(1);
    }
    new_array->capacity = capacity;
    return new_array;
}
ObjArray* insertArray(ObjArray* array, int index, Value value) {
    if (index < 0) {
        index = array->length + index;
    }
    if (index > array->length) {
        return array;
    }
    if (array->length == array->capacity) {
        array = reallocArray(array, array->capacity * 2);
        array->capacity *= 2;
    }
    array->values[index] = value;
    array->length++;
    return array;
}
Value getArray(ObjArray* array, int index) {
    if (index < 0) {
        index = array->length + index;
    }
    if (index >= array->length) {
        return NIL_VAL;
    }
    return array->values[index];
}
Value removeArray(ObjArray* array, int index); // shift values, might shrink array, return old value or (nil?)

void freeObject(Obj* obj) {
    switch (obj->type) {
        case OBJ_STRING: {
            ObjString* string = (ObjString*)obj;
            free(string);
            break;
        }
        case OBJ_ARRAY: {
            ObjArray* array = (ObjArray*)obj;
            free(array);
            break;
        }
        case OBJ_HASHMAP: {
            ObjHashmap* hashmap = (ObjHashmap*)obj;
            hashmap_free(&hashmap->map);
            free(hashmap);
            break;
        }
    }
}
