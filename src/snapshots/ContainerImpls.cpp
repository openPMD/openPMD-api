#include "openPMD/snapshots/ContainerImpls.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include <stdexcept>

namespace openPMD
{
auto StatefulSnapshotsContainer::begin() -> iterator
{
    return m_begin();
}
auto StatefulSnapshotsContainer::end() -> iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new SeriesIterator()});
}
auto StatefulSnapshotsContainer::begin() const -> const_iterator
{
    throw error::WrongAPIUsage(
        "Const iteration not possible on a stateful container/iterator.");
}
auto StatefulSnapshotsContainer::end() const -> const_iterator
{
    throw error::WrongAPIUsage(
        "Const iteration not possible on a stateful container/iterator.");
}
} // namespace openPMD
