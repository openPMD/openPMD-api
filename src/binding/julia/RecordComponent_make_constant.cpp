// RecordComponent_make_constant

#include "defs.hpp"

void define_julia_RecordComponent_make_constant(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type) {
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("make_constant1_" NAME, &RecordComponent::makeConstant<TYPE>);
  { FORALL_SCALAR_OPENPMD_TYPES }
#undef USE_TYPE
}
