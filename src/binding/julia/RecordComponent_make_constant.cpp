/* Bindings for RecordComponent::make_constant
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

void define_julia_RecordComponent_make_constant(
    jlcxx::Module & /*mod*/, jlcxx::TypeWrapper<RecordComponent> &type)
{
#define USE_TYPE(NAME, ENUM, TYPE)                                             \
    type.method(                                                               \
        "cxx_make_constant_" NAME, &RecordComponent::makeConstant<TYPE>);
    {
        FORALL_SCALAR_OPENPMD_TYPES(USE_TYPE)
    }
#undef USE_TYPE
}
