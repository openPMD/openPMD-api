#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"

namespace openPMD
{
using value_type =
    Container<Iteration, Iteration::IterationIndex_t>::value_type;
auto stateful_to_opaque(StatefulIterator const &it)
    -> OpaqueSeriesIterator<value_type>;
} // namespace openPMD
