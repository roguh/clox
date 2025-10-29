#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)

#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (AS_STRING(value)->chars)
#define STRING_LENGTH(value) (AS_STRING(value)->length)

#define AS_ARRAY(value) (((ObjArray*)AS_OBJ(value)))
#define ARRAY_LENGTH(value) (AS_ARRAY(value)->length)

typedef enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_HASHMAP,
    // TODO OBJ_ARRAY, OBJ_INT_ARRAY, OBJ_DOUBLE_ARRAY
    // TODO OBJ_HASHMAP
} ObjType;

struct Obj {
    ObjType type;
    struct Obj* next;
};

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
    Value values[];
};

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjString* copyString(const char* chars, size_t length);
ObjString* allocateString(char* chars, size_t length, size_t hash);

ObjArray* allocateArray(size_t capacity);
ObjArray* reallocArray(ObjArray* array, size_t capacity);
ObjArray* insertArray(ObjArray* array, int index, Value value); // might grow array, return old value or (nil?)
Value getArray(ObjArray* array, int index); // bounds check!!!
Value removeArray(ObjArray* array, int index); // shift values, might shrink array, return old value or (nil?)

void freeObject(Obj* obj);

#endif
