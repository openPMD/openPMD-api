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
  define_julia_RecordComponent_make_constant(mod, type);
  type.method(
      "make_empty1",
      static_cast<RecordComponent &(RecordComponent::*)(Datatype, uint8_t)>(
          &RecordComponent::makeEmpty));
  type.method("empty1", &RecordComponent::empty);
  define_julia_RecordComponent_copy_chunk(mod, type);
  define_julia_RecordComponent_load_chunk(mod, type);
  define_julia_RecordComponent_load_chunk_buffer(mod, type);
  define_julia_RecordComponent_store_chunk(mod, type);
  define_julia_RecordComponent_store_chunk_copy(mod, type);
  type.method("SCALAR1", []() { return RecordComponent::SCALAR; });
}
