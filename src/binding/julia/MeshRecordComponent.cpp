// MeshRecordComponent

#include "defs.hpp"

// Define supertype relationships
namespace jlcxx {
template <> struct SuperType<MeshRecordComponent> {
  typedef RecordComponent type;
};
} // namespace jlcxx

void define_julia_MeshRecordComponent(jlcxx::Module &mod) {
  auto type = mod.add_type<MeshRecordComponent>(
      "MeshRecordComponent", jlcxx::julia_base_type<RecordComponent>());

  type.method("position1", &MeshRecordComponent::position<double>);
  type.method("set_position1!", &MeshRecordComponent::setPosition<double>);
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("make_constant1_" NAME, &MeshRecordComponent::makeConstant<TYPE>);
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
}
