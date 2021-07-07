// RecordComponent_copy_chunk

#include "defs.hpp"

void define_julia_RecordComponent_copy_chunk(
    jlcxx::Module &mod, jlcxx::TypeWrapper<RecordComponent> &type) {
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("copy_chunk1_" NAME,                                             \
              [](const std::shared_ptr<TYPE> &ptr, std::vector<TYPE> &data) {  \
                const TYPE *dataptr = ptr.get();                               \
                std::size_t datasize = data.size();                            \
                std::copy(dataptr, dataptr + datasize, data.begin());          \
              });
  { FORALL_SCALAR_OPENPMD_TYPES }
#undef USE_TYPE
}
