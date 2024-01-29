#include "openPMD/snapshots/ContainerImpls.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include <stdexcept>

namespace openPMD
{
StatefulSnapshotsContainer::StatefulSnapshotsContainer(
    std::function<OpaqueSeriesIterator<value_type>()> begin)
    : m_begin(std::move(begin))
{}
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

RandomAccessIteratorContainer::RandomAccessIteratorContainer(
    Container<Iteration, key_type> cont)
    : m_cont(std::move(cont))
{}
auto RandomAccessIteratorContainer::begin() -> iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.begin())});
}
auto RandomAccessIteratorContainer::end() -> iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.end())});
}
auto RandomAccessIteratorContainer::begin() const -> const_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.begin())});
}
auto RandomAccessIteratorContainer::end() const -> const_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.end())});
}
} // namespace openPMD
