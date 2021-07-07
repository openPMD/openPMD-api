// RecordComponent_store_chunk

#include "defs.hpp"

void define_julia_RecordComponent_store_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type) {
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("store_chunk1_" NAME,                                            \
              static_cast<void (RecordComponent::*)(std::shared_ptr<TYPE>,     \
                                                    Offset, Extent)>(          \
                  &RecordComponent::storeChunk<TYPE>));
  { FORALL_SCALAR_OPENPMD_TYPES }
#undef USE_TYPE
}
