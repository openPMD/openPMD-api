// Attribute

#include "defs.hpp"

void define_julia_Attribute(jlcxx::Module &mod)
{
    auto type = mod.add_type<Attribute>("CXX_Attribute");

    type.method("cxx_dtype", [](const Attribute &attr) { return attr.dtype; });

#define USE_TYPE(NAME, ENUM, TYPE)                                             \
    type.method("cxx_get_" NAME, &Attribute::get<TYPE>);
    {
        FORALL_OPENPMD_TYPES(USE_TYPE)
    }
#undef USE_TYPE
}
