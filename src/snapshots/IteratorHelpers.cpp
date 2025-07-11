#include "openPMD/snapshots/IteratorHelpers.hpp"

namespace openPMD
{
using value_type =
    Container<Iteration, Iteration::IterationIndex_t>::value_type;
auto stateful_to_opaque(StatefulIterator const &it) -> OpaqueSeriesIterator<
    Container<Iteration, Iteration::IterationIndex_t>::value_type>
{
    return from_concrete_iterator<
        StatefulIterator,
        Container<Iteration, Iteration::IterationIndex_t>::value_type>(it);
}
} // namespace openPMD
