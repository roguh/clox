#ifndef clox_object_h
#define clox_object_h

#include "common.h"
#include "value.h"

#define OBJ_TYPE(value) (AS_OBJ(value)->type)
#define IS_STRING(value) isObjType(value, OBJ_STRING)
#define AS_STRING(value) ((ObjString*)AS_OBJ(value))
#define AS_CSTRING(value) (((ObjString*)AS_OBJ(value))->chars)
#define STRING_LENGTH(value) (((ObjString*)AS_OBJ(value))->length)

typedef enum {
    OBJ_STRING,
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

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && AS_OBJ(value)->type == type;
}

ObjString* copyString(const char* chars, size_t length);
ObjString* allocateString(char* chars, size_t length, size_t hash);
void freeObject(Obj* obj);

#endif
