/* Copyright 2025 Franz Poeschel
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
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
auto OpaqueSeriesIterator<value_type>::operator*() -> value_type &
{
    return m_internal_iterator->dereference_operator();
}
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
auto AbstractSnapshotsContainer::at(key_type const &key) -> mapped_type &
{
    return const_cast<mapped_type &>(
        static_cast<AbstractSnapshotsContainer const *>(this)->at(key));
}
} // namespace openPMD
