/* Bindings for std::shared_ptr
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

void define_julia_shared_ptr(jlcxx::Module &mod)
{
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
    mod.method(                                                                \
        "create_aliasing_shared_ptr_" NAME,                                    \
        &create_aliasing_shared_ptr<TYPE>);
    {
        FORALL_SCALAR_OPENPMD_TYPES(USE_TYPE)
    }
#undef USE_TYPE
}
