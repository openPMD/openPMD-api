// Attribute

#include "defs.hpp"

void define_julia_Attribute(jlcxx::Module &mod) {
  auto type = mod.add_type<Attribute>("Attribute");

  type.method("dtype1", [](const Attribute &attr) { return attr.dtype; });

#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("get1_" NAME, &Attribute::get<TYPE>);
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
}
