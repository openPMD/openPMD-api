/* Bindings for Datset
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

void define_julia_Dataset(jlcxx::Module &mod)
{
    auto type = mod.add_type<Dataset>("Dataset");

    type.constructor<Datatype, Extent>();
    type.constructor<Datatype, Extent, const std::string &>();
    type.constructor<Extent>();

    type.method("cxx_extend!", &Dataset::extend);
    type.method("cxx_extent", [](const Dataset &d) { return d.extent; });
    type.method("cxx_dtype", [](const Dataset &d) { return d.dtype; });
    type.method("cxx_rank", [](const Dataset &d) { return d.rank; });
    type.method("options", [](const Dataset &d) { return d.options; });
}
