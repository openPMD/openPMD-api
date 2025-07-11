#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"

namespace openPMD
{
auto stateful_to_opaque(StatefulIterator const &it) -> OpaqueSeriesIterator<
    Container<Iteration, Iteration::IterationIndex_t>::value_type>;

template <
    typename ConcreteIteratorClass,
    typename ValueType,
    typename... ConstructorArgs>
auto from_concrete_iterator(ConstructorArgs &&...args)
    -> OpaqueSeriesIterator<ValueType>
{
    return OpaqueSeriesIterator<ValueType>(
        std::unique_ptr<DynamicSeriesIterator<ValueType>>{
            new ConcreteIteratorClass(std::forward<ConstructorArgs>(args)...)});
}
} // namespace openPMD
