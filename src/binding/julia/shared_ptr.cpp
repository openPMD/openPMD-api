// shared_ptr

#include "defs.hpp"

void define_julia_shared_ptr(jlcxx::Module &mod)
{
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
    mod.method(                                                                \
        "create_aliasing_shared_ptr_" NAME,                                    \
        &create_aliasing_shared_ptr<TYPE>);
    {
        FORALL_SCALAR_OPENPMD_TYPES(USE_TYPE)
    }
#undef USE_TYPE
}
