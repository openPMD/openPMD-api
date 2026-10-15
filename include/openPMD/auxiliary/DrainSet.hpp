/* Copyright 2026 Franz Poeschel
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

#include <cstddef>
#include <iterator>

namespace openPMD::auxiliary
{

/*
 * std::set<> normally allows only const access to its members, because
 * modification of the value would generally also invalidate the value's lookup
 * logic in the set. However, when clearing a set, it is fine to modify values,
 * because we are discarding the set anyway. Below is a helper to simplify doing
 * this automatically in a for loop.
 */

template <class Set>
class drain_view
{
    Set &set_;

    class iterator
    {
        Set *m_set;
        typename Set::node_type m_node;

    public:
        using value_type = typename Set::value_type;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        explicit iterator(Set *set) : m_set(set)
        {
            if (m_set && !m_set->empty())
                m_node = m_set->extract(m_set->begin());
        }

        auto operator*() const -> value_type &
        {
            return m_node.value();
        }

        auto operator->() const -> value_type *
        {
            return &m_node.value();
        }

        auto operator++() -> iterator &
        {
            m_node = {};

            if (!m_set->empty())
                m_node = m_set->extract(m_set->begin());

            return *this;
        }

        auto operator==(iterator const &other) const -> bool
        {
            return this->m_node.empty() == other.m_node.empty();
        }

        auto operator!=(iterator const &other) const -> bool
        {
            return !operator==(other);
        }
    };

public:
    explicit drain_view(Set &set) : set_(set)
    {}

    auto begin() -> iterator
    {
        return iterator(&set_);
    }

    auto end() -> iterator
    {
        return iterator{nullptr};
    }
};

template <class Set>
drain_view<Set> drain(Set &set)
{
    return drain_view<Set>(set);
}
} // namespace openPMD::auxiliary
