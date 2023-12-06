/* Bindings for RecordComponent::make_constant
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

namespace
{
struct method_make_constant
{
    template <typename T>
    void call(jlcxx::TypeWrapper<RecordComponent> &type) const
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
    forallScalarJuliaTypes(method_make_constant(), type);
}
