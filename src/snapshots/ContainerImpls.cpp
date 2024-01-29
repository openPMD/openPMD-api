#include "openPMD/snapshots/ContainerImpls.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include <stdexcept>

namespace openPMD
{
auto StatefulSnapshotsContainer::begin() -> iterator_t
{
    return m_begin();
}
auto StatefulSnapshotsContainer::end() -> iterator_t
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator>{new SeriesIterator()});
}
} // namespace openPMD
