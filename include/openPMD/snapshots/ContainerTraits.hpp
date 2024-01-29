#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/snapshots/IteratorTraits.hpp"
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
} // namespace openPMD
