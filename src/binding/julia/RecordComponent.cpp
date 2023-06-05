// RecordComponent

#include "defs.hpp"

#include <algorithm>
#include <memory>
#include <type_traits>
#include <vector>

// Define supertype relationships
namespace jlcxx
{
template <>
struct SuperType<RecordComponent>
{
    using type = BaseRecordComponent;
};
} // namespace jlcxx

void define_julia_RecordComponent(jlcxx::Module &mod)
{

    // RecordComponent::Allocation
    mod.add_bits<RecordComponent::Allocation>(
        "Allocation", jlcxx::julia_type("CppEnum"));
    jlcxx::stl::apply_stl<RecordComponent::Allocation>(mod);

    mod.set_const("ALLOCATION_USER", RecordComponent::Allocation::USER);
    mod.set_const("ALLOCATION_API", RecordComponent::Allocation::API);
    mod.set_const("ALLOCATION_AUTO", RecordComponent::Allocation::AUTO);

    auto type = mod.add_type<RecordComponent>(
        "CXX_RecordComponent", jlcxx::julia_base_type<BaseRecordComponent>());

    type.method("cxx_set_unit_SI!", &RecordComponent::setUnitSI);
    type.method("cxx_reset_dataset!", &RecordComponent::resetDataset);
    type.method("cxx_get_dimensionality", &RecordComponent::getDimensionality);
    type.method("cxx_get_extent", &RecordComponent::getExtent);
    define_julia_RecordComponent_make_constant(mod, type);
    type.method(
        "cxx_make_empty",
        static_cast<RecordComponent &(RecordComponent::*)(Datatype, uint8_t)>(
            &RecordComponent::makeEmpty));
    type.method("cxx_isempty", &RecordComponent::empty);
    define_julia_RecordComponent_load_chunk(mod, type);
    define_julia_RecordComponent_store_chunk(mod, type);
    type.method("cxx_SCALAR", []() { return RecordComponent::SCALAR; });
}
