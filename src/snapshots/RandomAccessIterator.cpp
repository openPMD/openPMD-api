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
#include "openPMD/snapshots/RandomAccessIterator.hpp"
namespace openPMD
{
template <typename iterator_t>
inline RandomAccessIterator<iterator_t>::RandomAccessIterator(iterator_t it)
    : m_it(it)
{}

template <typename iterator_t>
RandomAccessIterator<iterator_t>::~RandomAccessIterator() = default;

template <typename iterator_t>
#ifdef __HIPCC__ // ROCm 6.2.4 issue, see #1797
__host__
#endif
RandomAccessIterator<iterator_t>::RandomAccessIterator(
    RandomAccessIterator const &other) = default;

template <typename iterator_t>
#ifdef __HIPCC__ // ROCm 6.2.4 issue, see #1797
__host__
#endif
RandomAccessIterator<iterator_t>::RandomAccessIterator(
    RandomAccessIterator
        &&other) noexcept(noexcept(iterator_t(std::declval<iterator_t &&>()))) =
    default;

template <typename iterator_t>
RandomAccessIterator<iterator_t> &RandomAccessIterator<iterator_t>::operator=(
    RandomAccessIterator const &other) = default;

template <typename iterator_t>
RandomAccessIterator<iterator_t> &RandomAccessIterator<iterator_t>::operator=(
    RandomAccessIterator
        &&other) noexcept(noexcept(std::declval<iterator_t>().
                                   operator=(std::declval<iterator_t &&>()))) =
    default;

template <typename iterator_t>
auto RandomAccessIterator<iterator_t>::operator*() -> value_type &
{
    return *m_it;
}

template <typename iterator_t>
auto RandomAccessIterator<iterator_t>::operator*() const -> value_type const &
{
    return *m_it;
}

template <typename iterator_t>
auto RandomAccessIterator<iterator_t>::operator++() -> RandomAccessIterator &
{
    ++m_it;
    return *this;
}

template <typename iterator_t>
auto RandomAccessIterator<iterator_t>::operator--() -> RandomAccessIterator &
{
    --m_it;
    return *this;
}

template <typename iterator_t>
auto RandomAccessIterator<iterator_t>::operator++(int i) -> RandomAccessIterator
{
    return parent_t::default_increment_operator(i);
}

template <typename iterator_t>
auto RandomAccessIterator<iterator_t>::operator--(int i) -> RandomAccessIterator
{
    return parent_t::default_decrement_operator(i);
}

template <typename iterator_t>
auto RandomAccessIterator<iterator_t>::operator==(
    RandomAccessIterator const &other) const -> bool
{
    return m_it == other.m_it;
}

using iterator = Container<Iteration, Iteration::IterationIndex_t>::iterator;
using const_iterator =
    Container<Iteration, Iteration::IterationIndex_t>::const_iterator;
using reverse_iterator =
    Container<Iteration, Iteration::IterationIndex_t>::reverse_iterator;
using const_reverse_iterator =
    Container<Iteration, Iteration::IterationIndex_t>::const_reverse_iterator;
template class RandomAccessIterator<iterator>;
template class RandomAccessIterator<const_iterator>;
template class RandomAccessIterator<reverse_iterator>;
template class RandomAccessIterator<const_reverse_iterator>;
} // namespace openPMD
