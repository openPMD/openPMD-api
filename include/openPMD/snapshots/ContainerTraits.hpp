#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/snapshots/IteratorTraits.hpp"
namespace openPMD
{
template <typename value_type_in>
class OpaqueSeriesIterator
    : public AbstractSeriesIterator<
          OpaqueSeriesIterator<value_type_in>,
          value_type_in>
{
private:
    friend class Series;
    friend class StatefulSnapshotsContainer;
    friend class RandomAccessIteratorContainer;
    using parent_t = AbstractSeriesIterator<OpaqueSeriesIterator>;
    // no shared_ptr since copied iterators should not share state
    std::unique_ptr<DynamicSeriesIterator<value_type_in>> m_internal_iterator;

    OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type_in>> internal_iterator)
        : m_internal_iterator(std::move(internal_iterator))
    {}

public:
    OpaqueSeriesIterator(OpaqueSeriesIterator const &other)
        : m_internal_iterator(other.m_internal_iterator->clone())
    {}
    OpaqueSeriesIterator(OpaqueSeriesIterator &&other) noexcept = default;
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator const &other)
    {
        m_internal_iterator = other.m_internal_iterator->clone();
        return *this;
    }
    OpaqueSeriesIterator &
    operator=(OpaqueSeriesIterator &&other) noexcept = default;

    ~OpaqueSeriesIterator() = default;

    // dereference
    using parent_t::operator*;
    using value_type = value_type_in;

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
    using key_type = Iteration::IterationIndex_t;
    using value_type = Container<Iteration, key_type>::value_type;
    using iterator = OpaqueSeriesIterator<value_type>;
    using const_iterator = OpaqueSeriesIterator<value_type const>;
    virtual iterator begin() = 0;
    virtual const_iterator begin() const = 0;
    virtual iterator end() = 0;
    virtual const_iterator end() const = 0;
};
} // namespace openPMD
