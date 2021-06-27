// RecordComponent_store_chunk

#include "defs.hpp"

void define_julia_RecordComponent_store_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type) {
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("store_chunk1_" NAME,                                            \
              static_cast<void (RecordComponent::*)(std::shared_ptr<TYPE>,     \
                                                    Offset, Extent)>(          \
                  &RecordComponent::storeChunk<TYPE>));
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("store_chunk_copy1_" NAME, [](RecordComponent &comp,             \
                                            std::vector<TYPE> data,            \
                                            Offset offset, Extent extent) {    \
    std::shared_ptr<TYPE> ptr(capture_vector(std::move(data)));                \
    comp.storeChunk(std::move(ptr), std::move(offset), std::move(extent));     \
  });
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
}
