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
      "CXX_BaseRecordComponent", jlcxx::julia_base_type<Attributable>());

  type.method("unit_SI", &BaseRecordComponent::unitSI);
  type.method("cxx_reset_datatype!", &BaseRecordComponent::resetDatatype);
  type.method("cxx_get_datatype", &BaseRecordComponent::getDatatype);
  type.method("is_constant", &BaseRecordComponent::constant);
  // TODO: availableChunks
}
