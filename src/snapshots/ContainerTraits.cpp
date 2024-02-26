#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/Iteration.hpp"
#include <optional>

namespace openPMD
{
// constructors
template <typename value_type>
OpaqueSeriesIterator<value_type>::OpaqueSeriesIterator(
    std::unique_ptr<DynamicSeriesIterator<value_type>> internal_iterator)
    : m_internal_iterator(std::move(internal_iterator))
{}

// copy/move constructor
template <typename value_type>
OpaqueSeriesIterator<value_type>::OpaqueSeriesIterator(
    OpaqueSeriesIterator const &other)
    : m_internal_iterator(other.m_internal_iterator->clone())
{}
template <typename value_type>
OpaqueSeriesIterator<value_type>::OpaqueSeriesIterator(
    OpaqueSeriesIterator &&other) noexcept = default;
template <typename value_type>

// copy/move assignment
OpaqueSeriesIterator<value_type> &
OpaqueSeriesIterator<value_type>::operator=(OpaqueSeriesIterator const &other)
{
    m_internal_iterator = other.m_internal_iterator->clone();
    return *this;
}
template <typename value_type>
OpaqueSeriesIterator<value_type> &OpaqueSeriesIterator<value_type>::operator=(
    OpaqueSeriesIterator &&other) noexcept = default;

// destructor
template <typename value_type>
OpaqueSeriesIterator<value_type>::~OpaqueSeriesIterator() = default;

// dereference
template <typename value_type>
auto OpaqueSeriesIterator<value_type>::operator*() const -> value_type const &
{
    return m_internal_iterator->dereference_operator();
}

// increment/decrement
template <typename value_type>
auto OpaqueSeriesIterator<value_type>::operator++() -> OpaqueSeriesIterator &
{
    m_internal_iterator->increment_operator();
    return *this;
}
template <typename value_type>
auto OpaqueSeriesIterator<value_type>::operator--() -> OpaqueSeriesIterator &
{
    m_internal_iterator->decrement_operator();
    return *this;
}
template <typename value_type>
auto OpaqueSeriesIterator<value_type>::operator++(int) -> OpaqueSeriesIterator
{
    auto prev = *this;
    ++(*this);
    return prev;
}
template <typename value_type>
auto OpaqueSeriesIterator<value_type>::operator--(int) -> OpaqueSeriesIterator
{
    auto prev = *this;
    --(*this);
    return prev;
}

// comparison
template <typename value_type>
auto OpaqueSeriesIterator<value_type>::operator==(
    OpaqueSeriesIterator const &other) const -> bool
{
    return m_internal_iterator->equality_operator(*other.m_internal_iterator);
}

using value_type =
    Container<Iteration, Iteration::IterationIndex_t>::value_type;
template class OpaqueSeriesIterator<value_type>;
template class OpaqueSeriesIterator<value_type const>;

AbstractSnapshotsContainer::~AbstractSnapshotsContainer() = default;

auto AbstractSnapshotsContainer::currentIteration()
    -> std::optional<value_type *>
{
    if (auto maybe_value = static_cast<AbstractSnapshotsContainer const *>(this)
                               ->currentIteration();
        maybe_value.has_value())
    {
        return {const_cast<value_type *>(*maybe_value)};
    }
    else
    {
        return std::nullopt;
    }
}
auto AbstractSnapshotsContainer::currentIteration() const
    -> std::optional<value_type const *>
{
    return std::nullopt;
}

auto AbstractSnapshotsContainer::at(key_type const &key) -> mapped_type &
{
    return const_cast<mapped_type &>(
        static_cast<AbstractSnapshotsContainer const *>(this)->at(key));
}
} // namespace openPMD
