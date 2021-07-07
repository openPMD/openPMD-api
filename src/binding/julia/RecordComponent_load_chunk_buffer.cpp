// RecordComponent_load_chunk_buffer

#include "defs.hpp"

void define_julia_RecordComponent_load_chunk_buffer(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type) {
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("load_chunk1_" NAME, [](RecordComponent &comp,                   \
                                      std::vector<TYPE> &buffer,               \
                                      Offset offset, Extent extent) {          \
    std::shared_ptr<TYPE> ptr(capture_vector_as_buffer(buffer));               \
    comp.loadChunk(std::move(ptr), std::move(offset), std::move(extent));      \
  });
  { FORALL_SCALAR_OPENPMD_TYPES }
#undef USE_TYPE
}
