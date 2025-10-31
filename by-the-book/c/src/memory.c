#include <stdio.h>
#include <stdlib.h>

#include "object.h"
#include "memory.h"
#include "vm.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize) {
    if (!newSize) {
        free(pointer);
        return NULL;
    }
    // TODO calloc not malloc? OR memset any new memory to 0
    return realloc(pointer, newSize);
}

void freeObjects(void) {
    Obj* obj = vm.objects;
    while (obj != NULL) {
        Obj* next = obj->next;
        freeObject(obj);
        obj = next;
    }
}
