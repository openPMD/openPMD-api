/* Copyright 2021 Franz Poeschel
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
#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/snapshots/IteratorTraits.hpp"
#include <utility>
namespace openPMD
{
namespace detail
{
    template <typename iterator_t>
    using iterator_to_value_type =
        // do NOT remove const from type
        std::remove_reference_t<decltype(*std::declval<iterator_t>())>;
}

template <typename iterator_t>
class RandomAccessIterator
    : public AbstractSeriesIterator<
          RandomAccessIterator<iterator_t>,
          detail::iterator_to_value_type<iterator_t>>
{
private:
    friend class RandomAccessIteratorContainer;
    using parent_t = AbstractSeriesIterator<
        RandomAccessIterator<iterator_t>,
        detail::iterator_to_value_type<iterator_t>>;

    RandomAccessIterator(iterator_t it);

    iterator_t m_it;

public:
    using typename parent_t::value_type;

    ~RandomAccessIterator() override;

    RandomAccessIterator(RandomAccessIterator const &other);
    RandomAccessIterator(RandomAccessIterator &&other) noexcept(
        noexcept(iterator_t(std::declval<iterator_t &&>())));

    RandomAccessIterator &operator=(RandomAccessIterator const &other);
    RandomAccessIterator &
    operator=(RandomAccessIterator &&other) noexcept(noexcept(
        std::declval<iterator_t>().operator=(std::declval<iterator_t &&>())));

    auto operator*() -> value_type &;
    auto operator*() const -> value_type const &;

    auto operator++() -> RandomAccessIterator &;
    auto operator--() -> RandomAccessIterator &;
    auto operator++(int) -> RandomAccessIterator;
    auto operator--(int) -> RandomAccessIterator;

    using parent_t::operator!=;
    bool operator==(RandomAccessIterator const &other) const;
};
} // namespace openPMD
