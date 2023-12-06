/* Bindings for RecordComponent::store_chunk
 *
 * File authors: Erik Schnetter
 * License: LGPL-3.0-or-later
 */

#include "defs.hpp"

namespace
{
struct method_store_chunk
{
    template <typename T>
    void call(jlcxx::TypeWrapper<RecordComponent> &type) const
    {
        type.method(
            "cxx_store_chunk_" + datatypeToString(determineDatatype<T>()),
            overload_cast<std::shared_ptr<T>, Offset, Extent>(
                &RecordComponent::storeChunk<T>));
    }
};
} // namespace

void define_julia_RecordComponent_store_chunk(
    jlcxx::Module & /*mod*/, jlcxx::TypeWrapper<RecordComponent> &type)
{
    forallScalarJuliaTypes(method_store_chunk(), type);
}
