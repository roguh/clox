#include <complex.h>

#include "value.h"
#include "vm.h"

static void defineConstant(const char* name, Value val) {
    ObjString* _name = copyString(name, strlen(name));
    hashmap_add(
        &vm.globals,
        OBJ_VAL(_name),
        val
    );
}

// TODO arity and type check
#define DEFINE_NATIVE(name, convert) \
    Value FFI_##name(int argCount, Value* args) { \
        return convert(name(AS_FCOMPLEX(args[0]))); \
    }
#define DEFINE_CNATIVE(name) DEFINE_NATIVE(name, FCOMPLEX_VAL)
#define DEFINE_CNATIVE2(name) \
    Value FFI_##name(int argCount, Value* args) { \
        return FCOMPLEX_VAL(name(AS_FCOMPLEX(args[0]), AS_FCOMPLEX(args[1]))); \
    }


DEFINE_NATIVE(cabs, DOUBLE_VAL)
DEFINE_CNATIVE(cacos)
DEFINE_CNATIVE(cacosh)
DEFINE_NATIVE(carg, DOUBLE_VAL)
DEFINE_CNATIVE(casin)
DEFINE_CNATIVE(casinh)
DEFINE_CNATIVE(catan)
DEFINE_CNATIVE(catanh)
DEFINE_CNATIVE(ccos)
DEFINE_CNATIVE(ccosh)
DEFINE_CNATIVE(cexp)
DEFINE_NATIVE(cimag, DOUBLE_VAL)
DEFINE_CNATIVE(clog)
DEFINE_CNATIVE(conj)
DEFINE_CNATIVE(cproj)
DEFINE_CNATIVE(creal)
DEFINE_CNATIVE(csin)
DEFINE_CNATIVE(csinh)
DEFINE_CNATIVE(csqrt)
DEFINE_CNATIVE(ctan)
DEFINE_CNATIVE(ctanh)
DEFINE_CNATIVE2(cpow)

void defineComplexLib() {
#define ADD_NATIVE(name, arity) \
    defineConstant(#name, OBJ_VAL(newNative(copyString(#name, sizeof(#name)), arity, FFI_##name)));

    ADD_NATIVE(cabs, 1);
    ADD_NATIVE(cacos, 1);
    ADD_NATIVE(cacosh, 1);
    ADD_NATIVE(carg, 1);
    ADD_NATIVE(casin, 1);
    ADD_NATIVE(casinh, 1);
    ADD_NATIVE(catan, 1);
    ADD_NATIVE(catanh, 1);
    ADD_NATIVE(ccos, 1);
    ADD_NATIVE(ccosh, 1);
    defineConstant("cexp", OBJ_VAL(newNative(copyString("cexp", sizeof("cexp")), 1, FFI_cexp)));
    ADD_NATIVE(cimag, 1);
    ADD_NATIVE(clog, 1);
    ADD_NATIVE(conj, 1);
    ADD_NATIVE(cpow, 2);
    ADD_NATIVE(cproj, 1);
    ADD_NATIVE(creal, 1);
    ADD_NATIVE(csin, 1);
    ADD_NATIVE(csinh, 1);
    ADD_NATIVE(csqrt, 1);
    ADD_NATIVE(ctan, 1);
    ADD_NATIVE(ctanh, 1);
    defineConstant("I", FCOMPLEX_VAL(I));
}
