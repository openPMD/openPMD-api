// MeshRecordComponent

#include "defs.hpp"

// Define supertype relationships
namespace jlcxx
{
template <>
struct SuperType<MeshRecordComponent>
{
    using type = RecordComponent;
};
} // namespace jlcxx

namespace
{
struct UseType
{
    template <typename T>
    static void call(jlcxx::TypeWrapper<MeshRecordComponent> type)
    {
        type.method(
            "cxx_make_constant_" + datatypeToString(determineDatatype<T>()),
            &MeshRecordComponent::makeConstant<T>);
    }
};
} // namespace

void define_julia_MeshRecordComponent(jlcxx::Module &mod)
{
    auto type = mod.add_type<MeshRecordComponent>(
        "CXX_MeshRecordComponent", jlcxx::julia_base_type<RecordComponent>());

    type.method("cxx_position", &MeshRecordComponent::position<double>);
    type.method("cxx_set_position!", &MeshRecordComponent::setPosition<double>);
    forallJuliaTypes<UseType>(type);
}
