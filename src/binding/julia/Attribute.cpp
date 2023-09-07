/* Bindings for Attribute
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

namespace
{
struct method_get
{
    template <typename T>
    void call(jlcxx::TypeWrapper<Attribute> type) const
    {
        type.method(
            "cxx_get_" + datatypeToString(determineDatatype<T>()),
            &Attribute::get<T>);
    }
};
} // namespace

void define_julia_Attribute(jlcxx::Module &mod)
{
    auto type = mod.add_type<Attribute>("CXX_Attribute");

    type.method("cxx_dtype", [](const Attribute &attr) { return attr.dtype; });

    forallJuliaTypes(method_get(), type);
}
