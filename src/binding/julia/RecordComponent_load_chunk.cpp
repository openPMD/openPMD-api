// RecordComponent_load_chunk

#include "defs.hpp"

void define_julia_RecordComponent_load_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type) {
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("load_chunk1_" NAME, [](RecordComponent &comp,                   \
                                      std::vector<TYPE> &buffer,               \
                                      Offset offset, Extent extent) {          \
    std::shared_ptr<TYPE> ptr(capture_vector_as_buffer(buffer));               \
    comp.loadChunk(std::move(ptr), std::move(offset), std::move(extent));      \
  });
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method(                                                                 \
      "load_chunk1_" NAME,                                                     \
      static_cast<std::shared_ptr<TYPE> (RecordComponent::*)(Offset, Extent)>( \
          &RecordComponent::loadChunk<TYPE>));
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("copy_chunk1_" NAME,                                             \
              [](const std::shared_ptr<TYPE> &ptr, std::vector<TYPE> &data) {  \
                const TYPE *dataptr = ptr.get();                               \
                std::size_t datasize = data.size();                            \
                std::copy(dataptr, dataptr + datasize, data.begin());          \
              });
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
}
