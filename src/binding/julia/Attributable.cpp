/* Bindings for Attributable
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

namespace
{
struct method_set_attribute
{
    template <typename T>
    void call(jlcxx::TypeWrapper<Attributable> type) const
    {
        type.method(
            "cxx_set_attribute_" + datatypeToString(determineDatatype<T>()) +
                "!",
            &Attributable::setAttribute<T>);
    }
};
} // namespace

void define_julia_Attributable(jlcxx::Module &mod)
{
    auto type = mod.add_type<Attributable>("CXX_Attributable");

    forallJuliaTypes(method_set_attribute(), type);

    type.method("cxx_get_attribute", &Attributable::getAttribute);
    type.method("cxx_delete_attribute!", &Attributable::deleteAttribute);
    type.method("cxx_attributes", &Attributable::attributes);
    type.method("cxx_num_attributes", &Attributable::numAttributes);
    type.method("cxx_contains_attribute", &Attributable::containsAttribute);
    type.method("cxx_comment", &Attributable::comment);
    type.method("cxx_set_comment!", &Attributable::setComment);
    type.method(
        "cxx_series_flush", [](Attributable &attr) { attr.seriesFlush(); });
}
