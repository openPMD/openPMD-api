// RecordComponent_load_chunk

#include "defs.hpp"

void define_julia_RecordComponent_load_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type) {
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method(                                                                 \
      "load_chunk1_" NAME,                                                     \
      static_cast<std::shared_ptr<TYPE> (RecordComponent::*)(Offset, Extent)>( \
          &RecordComponent::loadChunk<TYPE>));
  { FORALL_SCALAR_OPENPMD_TYPES }
#undef USE_TYPE
}
