/* Copyright 2024 Franz Poeschel
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

#include "openPMD/SeriesIterator.hpp"
#include <functional>
#include <optional>

namespace openPMD
{

class OpaqueSeriesIterator : public AbstractSeriesIterator<OpaqueSeriesIterator>
{
private:
    friend class Series;
    using parent_t = AbstractSeriesIterator<OpaqueSeriesIterator>;
    // no shared_ptr since copied iterators should not share state
    std::unique_ptr<DynamicSeriesIterator> m_internal_iterator;

    OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator> internal_iterator)
        : m_internal_iterator(std::move(internal_iterator))
    {}

public:
    OpaqueSeriesIterator(OpaqueSeriesIterator const &other)
        : m_internal_iterator(other.m_internal_iterator->clone())
    {}
    OpaqueSeriesIterator(OpaqueSeriesIterator &&other) = default;
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator const &other)
    {
        m_internal_iterator = other.m_internal_iterator->clone();
        return *this;
    }
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator &&other) = default;

    ~OpaqueSeriesIterator() = default;

    // dereference
    using parent_t::operator*;
    inline value_type const &operator*() const
    {
        return m_internal_iterator->dereference_operator();
    }

    // member access
    inline value_type const &operator[](difference_type diff) const
    {
        return m_internal_iterator->index_operator(diff);
    }
    inline value_type &operator[](difference_type diff)
    {
        return m_internal_iterator->index_operator(diff);
    }

    // arithmetic random-access
    inline OpaqueSeriesIterator operator+(difference_type diff) const
    {
        return OpaqueSeriesIterator(m_internal_iterator->plus_operator(diff));
    }
    inline OpaqueSeriesIterator operator-(difference_type diff) const
    {
        return OpaqueSeriesIterator(m_internal_iterator->minus_operator(diff));
    }
    inline OpaqueSeriesIterator operator+(difference_type diff)
    {
        return OpaqueSeriesIterator(m_internal_iterator->plus_operator(diff));
    }
    inline OpaqueSeriesIterator operator-(difference_type diff)
    {
        return OpaqueSeriesIterator(m_internal_iterator->minus_operator(diff));
    }

    // increment/decrement
    OpaqueSeriesIterator &operator++()
    {
        m_internal_iterator->increment_operator();
        return *this;
    }
    OpaqueSeriesIterator &operator--()
    {
        m_internal_iterator->decrement_operator();
        return *this;
    }
    OpaqueSeriesIterator operator++(int)
    {
        auto prev = *this;
        ++(*this);
        return prev;
    }
    OpaqueSeriesIterator operator--(int)
    {
        auto prev = *this;
        --(*this);
        return prev;
    }

    // comparison
    inline difference_type operator-(OpaqueSeriesIterator const &other) const
    {
        return m_internal_iterator->difference_operator(
            *other.m_internal_iterator);
    }

    inline bool operator==(OpaqueSeriesIterator const &other) const
    {
        return m_internal_iterator->equality_operator(
            *other.m_internal_iterator);
    }
    inline bool operator<(OpaqueSeriesIterator const &other) const
    {
        return m_internal_iterator->less_than_operator(
            *other.m_internal_iterator);
    }
};

class Snapshots
{
private:
    friend class Series;

    std::function<OpaqueSeriesIterator()> m_begin;
    std::function<OpaqueSeriesIterator()> m_end;

    inline Snapshots(
        std::function<OpaqueSeriesIterator()> begin,
        std::function<OpaqueSeriesIterator()> end)
        : m_begin(std::move(begin)), m_end(std::move(end))
    {}

public:
    inline OpaqueSeriesIterator begin()
    {
        return m_begin();
    }
    inline OpaqueSeriesIterator end()
    {
        return m_end();
    }
};
} // namespace openPMD
