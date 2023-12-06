/* Bindings for RecordComponent::load_chunk
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

namespace
{
struct method_load_chunk
{
    template <typename T>
    void call(jlcxx::TypeWrapper<RecordComponent> &type) const
    {
        type.method(
            "cxx_load_chunk_" + datatypeToString(determineDatatype<T>()),
            overload_cast<std::shared_ptr<T>, Offset, Extent>(
                &RecordComponent::loadChunk<T>));
    }
};
} // namespace

void define_julia_RecordComponent_load_chunk(
    jlcxx::Module & /*mod*/, jlcxx::TypeWrapper<RecordComponent> &type)
{
    forallScalarJuliaTypes(method_load_chunk(), type);
}
