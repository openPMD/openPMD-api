// BaseRecordComponent

#include "defs.hpp"

// Define supertype relationships
namespace jlcxx
{
template <>
struct SuperType<BaseRecordComponent>
{
    using type = Attributable;
};
} // namespace jlcxx

void define_julia_BaseRecordComponent(jlcxx::Module &mod)
{
    auto type = mod.add_type<BaseRecordComponent>(
        "CXX_BaseRecordComponent", jlcxx::julia_base_type<Attributable>());

    type.method("cxx_unit_SI", &BaseRecordComponent::unitSI);
    type.method("cxx_reset_datatype!", &BaseRecordComponent::resetDatatype);
    type.method("cxx_get_datatype", &BaseRecordComponent::getDatatype);
    type.method("cxx_isconstant", &BaseRecordComponent::constant);
    type.method("cxx_available_chunks", &BaseRecordComponent::availableChunks);
}
