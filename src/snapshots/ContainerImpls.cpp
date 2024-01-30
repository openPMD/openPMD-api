#include "openPMD/snapshots/ContainerImpls.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include <memory>
#include <stdexcept>

namespace openPMD
{
namespace
{
    using value_type =
        Container<Iteration, Iteration::IterationIndex_t>::value_type;
    auto stateful_to_opaque(StatefulIterator const &it)
        -> OpaqueSeriesIterator<value_type>
    {
        std::unique_ptr<DynamicSeriesIterator<value_type>>
            internal_iterator_cloned{new StatefulIterator(it)};
        return OpaqueSeriesIterator<value_type>(
            std::move(internal_iterator_cloned));
    }
} // namespace

StatefulSnapshotsContainer::StatefulSnapshotsContainer(
    std::function<StatefulIterator *()> begin)
    : m_begin(std::move(begin))
{}
auto StatefulSnapshotsContainer::get() -> StatefulIterator *
{
    if (!m_bufferedIterator.has_value())
    {
        m_bufferedIterator = m_begin();
    }
    return *m_bufferedIterator;
}
auto StatefulSnapshotsContainer::get() const -> StatefulIterator const *
{
    return m_bufferedIterator.value_or(nullptr);
}
auto StatefulSnapshotsContainer::begin() -> iterator
{
    return stateful_to_opaque(*get());
}
auto StatefulSnapshotsContainer::end() -> iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new StatefulIterator()});
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
auto StatefulSnapshotsContainer::rbegin() -> iterator
{
    /*
     * @todo maybe can adapt std::reverse_iterator as soon as the stateful
     * iterator is powerful enough for this
     */
    throw std::runtime_error(
        "Reverse iteration not (yet) implemented on a stateful "
        "container/iterator.");
}
auto StatefulSnapshotsContainer::rend() -> iterator
{
    throw std::runtime_error(
        "Reverse iteration not (yet) implemented on a stateful "
        "container/iterator.");
}
auto StatefulSnapshotsContainer::rbegin() const -> const_iterator
{
    throw error::WrongAPIUsage(
        "Const iteration not possible on a stateful container/iterator.");
}
auto StatefulSnapshotsContainer::rend() const -> const_iterator
{
    throw error::WrongAPIUsage(
        "Const iteration not possible on a stateful container/iterator.");
}

bool StatefulSnapshotsContainer::empty() const
{
    return get()->operator bool();
}

auto StatefulSnapshotsContainer::at(key_type const &) const
    -> mapped_type const &
{
    throw std::runtime_error(
        "Item access not (yet) implemented on a stateful "
        "container/iterator.");
}
auto StatefulSnapshotsContainer::at(key_type const &) -> mapped_type &
{
    throw std::runtime_error(
        "Item access not (yet) implemented on a stateful "
        "container/iterator.");
}

auto StatefulSnapshotsContainer::operator[](key_type const &) -> mapped_type &
{
    throw std::runtime_error(
        "Item access not (yet) implemented on a stateful "
        "container/iterator.");
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
auto RandomAccessIteratorContainer::rbegin() -> reverse_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.rbegin())});
}
auto RandomAccessIteratorContainer::rend() -> reverse_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.end())});
}
auto RandomAccessIteratorContainer::rbegin() const -> const_reverse_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.rbegin())});
}
auto RandomAccessIteratorContainer::rend() const -> const_reverse_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.rend())});
}

bool RandomAccessIteratorContainer::empty() const
{
    return m_cont.empty();
}

auto RandomAccessIteratorContainer::at(key_type const &key) const
    -> mapped_type const &
{
    return m_cont.at(key);
}

auto RandomAccessIteratorContainer::operator[](key_type const &key)
    -> mapped_type &
{
    return m_cont[key];
}
} // namespace openPMD
