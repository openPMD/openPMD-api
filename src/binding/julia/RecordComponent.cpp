// RecordComponent

#include "defs.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>

// Define supertype relationships
namespace jlcxx {
template <> struct SuperType<RecordComponent> {
  typedef BaseRecordComponent type;
};
} // namespace jlcxx

namespace {
template <typename T> std::shared_ptr<T> capture_vector(std::vector<T> vec) {
  if constexpr (std::is_same_v<T, bool>) {
    // Copy the vector, because std::vector<bool> is special
    T *dataptr = new T[vec.size()];
    std::shared_ptr<T> ptr(dataptr, std::default_delete<T[]>());
    std::copy(vec.begin(), vec.end(), dataptr);
    return ptr;
  } else {
    // Capture the vector
    T *dataptr = vec.data();
    std::shared_ptr<T> ptr(dataptr, [vec = std::move(vec)](T *p) {
      /* We moved the vector into the anonymous function, and thus it will be
       * destructed when the anonymous function is destructed. There is no need
       * to call a destructor manually. */
    });
    return ptr;
  }
}
} // namespace

void define_julia_RecordComponent(jlcxx::Module &mod) {

  // RecordComponent::Allocation
  mod.add_bits<RecordComponent::Allocation>("Allocation",
                                            jlcxx::julia_type("CppEnum"));
  jlcxx::stl::apply_stl<RecordComponent::Allocation>(mod);

  mod.set_const("USER", RecordComponent::Allocation::USER);
  mod.set_const("API", RecordComponent::Allocation::API);
  mod.set_const("AUTO", RecordComponent::Allocation::AUTO);

  auto type = mod.add_type<RecordComponent>(
      "RecordComponent", jlcxx::julia_base_type<BaseRecordComponent>());

  type.method("set_unit_SI!", &RecordComponent::setUnitSI);
  type.method("reset_dataset!", &RecordComponent::resetDataset);
  type.method("get_dimensionality1", &RecordComponent::getDimensionality);
  type.method("get_extent1", &RecordComponent::getExtent);
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("make_constant1_" NAME, &RecordComponent::makeConstant<TYPE>);
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
  type.method(
      "make_empty1",
      static_cast<RecordComponent &(RecordComponent::*)(Datatype, uint8_t)>(
          &RecordComponent::makeEmpty));
  type.method("empty1", &RecordComponent::empty);
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method(                                                                 \
      "load_chunk1_" NAME,                                                     \
      static_cast<std::shared_ptr<TYPE> (RecordComponent::*)(Offset, Extent)>( \
          &RecordComponent::loadChunk<TYPE>));
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
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
  });                                                                          \
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
  type.method("SCALAR1", []() { return RecordComponent::SCALAR; });
}
