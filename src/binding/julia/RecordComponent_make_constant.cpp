// RecordComponent_make_constant

#include "defs.hpp"

namespace
{
struct UseType
{
    template <typename T>
    static void call(jlcxx::TypeWrapper<RecordComponent> &type)
    {
        type.method(
            "cxx_make_constant_" + datatypeToString(determineDatatype<T>()),
            &RecordComponent::makeConstant<T>);
    }
};
} // namespace

void define_julia_RecordComponent_make_constant(
    jlcxx::Module & /*mod*/, jlcxx::TypeWrapper<RecordComponent> &type)
{
    forallScalarJuliaTypes<UseType>(type);
}
