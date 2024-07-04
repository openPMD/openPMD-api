#include "openPMD/snapshots/IteratorHelpers.hpp"

namespace openPMD
{
using value_type =
    Container<Iteration, Iteration::IterationIndex_t>::value_type;
auto stateful_to_opaque(StatefulIterator const &it)
    -> OpaqueSeriesIterator<value_type>
{
    std::unique_ptr<DynamicSeriesIterator<value_type>> internal_iterator_cloned{
        new StatefulIterator(it)};
    return OpaqueSeriesIterator<value_type>(
        std::move(internal_iterator_cloned));
}
} // namespace openPMD
