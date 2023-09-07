/* Bindings for std::shared_ptr
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"
#include "openPMD/Datatype.hpp"
#include <jlcxx/module.hpp>

namespace
{
struct method_create_aliasing_shared_ptr
{
    template <typename T>
    void call(jlcxx::Module &mod) const
    {
        mod.method(
            "create_aliasing_shared_ptr_" +
                datatypeToString(determineDatatype<T>()),
            &create_aliasing_shared_ptr<T>);
    }
};
} // namespace

void define_julia_shared_ptr(jlcxx::Module &mod)
{
    forallScalarJuliaTypes(method_create_aliasing_shared_ptr(), mod);
}
