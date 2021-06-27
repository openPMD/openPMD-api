// BaseRecordComponent

#include "defs.hpp"

// Define supertype relationships
namespace jlcxx {
template <> struct SuperType<BaseRecordComponent> {
  typedef Attributable type;
};
} // namespace jlcxx

void define_julia_BaseRecordComponent(jlcxx::Module &mod) {

  auto type = mod.add_type<BaseRecordComponent>(
      "BaseRecordComponent", jlcxx::julia_base_type<Attributable>());

  type.method("unit_SI", &BaseRecordComponent::unitSI);
  type.method("reset_datatype1!", &BaseRecordComponent::resetDatatype);
  type.method("get_datatype1", &BaseRecordComponent::getDatatype);
  type.method("is_constant", &BaseRecordComponent::constant);
  // TODO: availableChunks
}
