#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_FUNCTION(value) isObjType(value, OBJ_FUNCTION)
#define AS_FUNCTION(value) ((ObjFunction*)AS_OBJ(value))

#define IS_NATIVE(value) isObjType(value, OBJ_NATIVE)
#define AS_NATIVE(value) (((ObjNative*)AS_OBJ(value)))

#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (AS_STRING(value)->chars)
#define STRING_LENGTH(value) (AS_STRING(value)->length)

#define IS_ARRAY(value) isObjType(value, OBJ_ARRAY)
#define AS_ARRAY(value) (((ObjArray*)AS_OBJ(value)))
#define ARRAY_LENGTH(value) (AS_ARRAY(value)->length)

#define IS_HASHMAP(value) isObjType(value, OBJ_HASHMAP)

typedef enum {
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_HASHMAP,
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

typedef struct {
    Obj obj;
    int arity;
    Chunk chunk;
    ObjString* name;
    // For debugging and documentation purposes
    ObjString* paramNames;
    ObjString* docs;
} ObjFunction;

typedef Value (*NativeFn)(int argCount, Value* args);

typedef struct {
    Obj obj;
    int arity;
    NativeFn function;
    ObjString* name;
    // For debugging and documentation purposes
    ObjString* paramNames;
    ObjString* docs;
} ObjNative;

struct ObjString {
    Obj obj;
    size_t length;
    size_t hash;
    char chars[];
};

struct ObjArray {
    Obj obj;
    size_t length;
    size_t capacity;
    Value* values;
};

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjFunction* newFunction(ObjString* name);
ObjNative* newNative(ObjString* name, int arity, NativeFn func);

ObjString* copyString(const char* chars, size_t length);
ObjString* allocateString(char* chars, size_t length, size_t hash);

ObjArray* allocateArray(size_t capacity);
void reallocArray(ObjArray* array, size_t capacity);
void insertArray(ObjArray* array, int index, Value value); // might grow array, return old value or (nil?)
Value getArray(ObjArray* array, int index); // bounds check!!!
Value removeArray(ObjArray* array, int index); // shift values, might shrink array, return old value or (nil?)

ObjHashmap* allocateHashmap(size_t capacity);

void freeObject(Obj* obj);

#endif
