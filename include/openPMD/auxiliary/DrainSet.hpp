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

/*
 * Sentinel type for drain_view::end(). Using a distinct sentinel type avoids
 * the hazard of constructing a begin()-style iterator just to perform an
 * end-of-range comparison: begin() extracts an element from the set, so
 * creating one as a temporary merely to compare against would silently consume
 * an element. With a sentinel, operator==(iterator, drain_sentinel) only checks
 * whether the iterator's current node is empty — no extraction occurs.
 */
struct drain_sentinel
{};

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

        explicit iterator(Set &set) : m_set(&set)
        {
            if (!m_set->empty())
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

        auto operator==(drain_sentinel const &) const -> bool
        {
            return m_node.empty();
        }

        auto operator!=(drain_sentinel const &) const -> bool
        {
            return !m_node.empty();
        }
    };

public:
    explicit drain_view(Set &set) : set_(set)
    {}

    auto begin() -> iterator
    {
        return iterator{set_};
    }

    auto end() -> drain_sentinel
    {
        return {};
    }
};

template <class Set>
drain_view<Set> drain(Set &set)
{
    return drain_view<Set>(set);
}
} // namespace openPMD::auxiliary
