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
#include <optional>

namespace openPMD
{

class OpaqueSeriesIterator : public AbstractSeriesIterator<OpaqueSeriesIterator>
{
private:
    // no shared_ptr since copied iterators should not share state
    std::unique_ptr<DynamicSeriesIterator> m_internal_iterator;

    OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator> internal_iterator)
        : m_internal_iterator(std::move(internal_iterator))
    {}

public:
    OpaqueSeriesIterator(OpaqueSeriesIterator &other)
        : m_internal_iterator(other.m_internal_iterator->clone())
    {}
    OpaqueSeriesIterator(OpaqueSeriesIterator &&other) = default;
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator &other)
    {
        m_internal_iterator = other.m_internal_iterator->clone();
        return *this;
    }
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator &&other) = default;

    ~OpaqueSeriesIterator() = default;

    // dereference
    inline value_type const &operator*() const override
    {
        return **m_internal_iterator;
    }

    // member access
    inline value_type const &operator[](difference_type diff) const override
    {
        return m_internal_iterator->operator[](diff);
    }
    inline value_type &operator[](difference_type diff) override
    {
        return m_internal_iterator->operator[](diff);
    }

    // arithmetic random-access
    inline OpaqueSeriesIterator operator+(difference_type diff) const override
    {
        return OpaqueSeriesIterator(m_internal_iterator->plus_operator(diff));
    }
    inline OpaqueSeriesIterator operator-(difference_type diff) const override
    {
        return OpaqueSeriesIterator(m_internal_iterator->minus_operator(diff));
    }
    inline OpaqueSeriesIterator operator+(difference_type diff) override
    {
        return OpaqueSeriesIterator(m_internal_iterator->plus_operator(diff));
    }
    inline OpaqueSeriesIterator operator-(difference_type diff) override
    {
        return OpaqueSeriesIterator(m_internal_iterator->minus_operator(diff));
    }

    // increment/decrement
    OpaqueSeriesIterator &operator++() override
    {
        m_internal_iterator->increment_operator();
        return *this;
    }
    OpaqueSeriesIterator &operator--() override
    {
        m_internal_iterator->decrement_operator();
        return *this;
    }
    OpaqueSeriesIterator operator++(int) override
    {
        auto prev = *this;
        ++(*this);
        return prev;
    }
    OpaqueSeriesIterator operator--(int) override
    {
        auto prev = *this;
        --(*this);
        return prev;
    }

    // comparison
    inline difference_type
    operator-(OpaqueSeriesIterator const &other) const override
    {
        return m_internal_iterator->difference_operator(
            *other.m_internal_iterator);
    }

    inline bool operator==(OpaqueSeriesIterator const &other) const override
    {
        return m_internal_iterator->equality_operator(
            *other.m_internal_iterator);
    }
    inline bool operator<(OpaqueSeriesIterator const &other) const override
    {
        return m_internal_iterator->less_than_operator(
            *other.m_internal_iterator);
    }
};

class Snapshots
{
    OpaqueSeriesIterator begin();
};
} // namespace openPMD
