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

ObjFunction* newFunction(ObjString* name) {
    ObjFunction* func = (ObjFunction*)allocateObj(sizeof(ObjFunction), OBJ_FUNCTION);
    initChunk(&func->chunk);
    func->arity = 0;
    func->name = name;
    return func;
}

ObjNative* newNative(ObjString* name, int arity, NativeFn func) {
    ObjNative* native = (ObjNative*)allocateObj(sizeof(ObjNative), OBJ_NATIVE);
    native->function = func;
    native->name = name;
    native->arity = arity;
    return native;
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

ObjHashmap* allocateHashmap(size_t capacity) {
    ObjHashmap* hashmap = (ObjHashmap*)allocateObj(sizeof(ObjHashmap), OBJ_HASHMAP);
    hashmap_init(&hashmap->map, capacity, hashAny);
    return hashmap;
}

ObjArray* allocateArray(size_t capacity) {
    ObjArray* array = (ObjArray*)allocateObj(sizeof(ObjArray), OBJ_ARRAY);
    array->capacity = capacity;
    array->length = 0;
    array->values = (Value*)calloc(capacity, sizeof(Value));
    return array;
}

void reallocArray(ObjArray* array, size_t capacity) {
    if (capacity == array->capacity) {
        return;
    }
    array->values = realloc(array->values, sizeof(Value) * capacity);
    if (array->values == NULL) {
        ERR_PRINT("Failed to reallocate array\n");
        exit(1);
    }
    array->capacity = capacity;
    return;
}

void insertArray(ObjArray* array, int index, Value value) {
    if (index < 0) {
        index = array->length + index;
    }
    if (index > array->length) {
        return;
    }
    if (array->length + 1 == array->capacity) {
        ERR_PRINT("Array: Growing capacity from %zu to %zu\n", array->capacity, array->capacity * 2);
        reallocArray(array, array->capacity * 2);
    }
    array->values[index] = value;
    array->length++;
    return;
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
        case OBJ_FUNCTION: {
            ObjFunction* func = (ObjFunction*)obj;
            freeChunk(&func->chunk);
            free(func);
            break;
        }
        case OBJ_NATIVE: {
            ObjNative* native = (ObjNative*)obj;
            free(native);
            break;
        }
        case OBJ_STRING: {
            ObjString* string = (ObjString*)obj;
            free(string);
            break;
        }
        case OBJ_ARRAY: {
            ObjArray* array = (ObjArray*)obj;
            free(array->values);
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
