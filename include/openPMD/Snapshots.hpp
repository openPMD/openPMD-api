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

#include "openPMD/Iteration.hpp"
#include "openPMD/SeriesIterator.hpp"
#include "openPMD/snapshots/RandomAccessIterator.hpp"
#include <functional>
#include <memory>
#include <optional>

namespace openPMD
{

class OpaqueSeriesIterator : public AbstractSeriesIterator<OpaqueSeriesIterator>
{
private:
    friend class Series;
    friend class StatefulSnapshotsContainer;
    friend class RandomAccessIteratorContainer;
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
    inline bool operator==(OpaqueSeriesIterator const &other) const
    {
        return m_internal_iterator->equality_operator(
            *other.m_internal_iterator);
    }
};

class AbstractSnapshotsContainer
{
public:
    using value_t = Iteration;
    using index_t = Iteration::IterationIndex_t;
    using iterator_t = OpaqueSeriesIterator;
    // using const_iterator_t = ...;
    virtual iterator_t begin() = 0;
    virtual iterator_t end() = 0;
};

class StatefulSnapshotsContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    std::function<OpaqueSeriesIterator()> m_begin;
    StatefulSnapshotsContainer(std::function<OpaqueSeriesIterator()> begin)
        : m_begin(std::move(begin))
    {}

public:
    iterator_t begin() override;
    iterator_t end() override;
};

/*
 * @todo how to deal with iteration::open() iteration::close() ?
 */
class RandomAccessIteratorContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    Container<value_t, index_t> m_cont;
    RandomAccessIteratorContainer(Container<value_t, index_t> cont)
        : m_cont(std::move(cont))
    {}

public:
    inline iterator_t begin() override
    {
        return OpaqueSeriesIterator(std::unique_ptr<DynamicSeriesIterator>{
            new RandomAccessIterator(m_cont.begin())});
    }
    inline iterator_t end() override
    {
        return OpaqueSeriesIterator(std::unique_ptr<DynamicSeriesIterator>{
            new RandomAccessIterator(m_cont.end())});
    }
};

class Snapshots
{
private:
    friend class Series;

    std::shared_ptr<AbstractSnapshotsContainer> m_snapshots;

    inline Snapshots(std::shared_ptr<AbstractSnapshotsContainer> snapshots)
        : m_snapshots(std::move(snapshots))
    {}

public:
    inline OpaqueSeriesIterator begin()
    {
        return m_snapshots->begin();
    }
    inline OpaqueSeriesIterator end()
    {
        return m_snapshots->end();
    }
};
} // namespace openPMD
