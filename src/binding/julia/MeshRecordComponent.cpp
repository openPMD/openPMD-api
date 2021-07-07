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
      "CXX_MeshRecordComponent", jlcxx::julia_base_type<RecordComponent>());

  type.method("cxx_position", &MeshRecordComponent::position<double>);
  type.method("cxx_set_position!", &MeshRecordComponent::setPosition<double>);
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
  type.method("cxx_make_constant_" NAME,                                       \
              &MeshRecordComponent::makeConstant<TYPE>);
  { FORALL_OPENPMD_TYPES }
#undef USE_TYPE
}
