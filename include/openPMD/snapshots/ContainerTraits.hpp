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

public:
    OpaqueSeriesIterator(std::unique_ptr<DynamicSeriesIterator<value_type_in>>
                             internal_iterator);

    OpaqueSeriesIterator(OpaqueSeriesIterator const &other);
    OpaqueSeriesIterator(OpaqueSeriesIterator &&other) noexcept;
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator const &other);
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator &&other) noexcept;

    ~OpaqueSeriesIterator();

    // dereference
    using parent_t::operator*;
    using value_type = value_type_in;

    value_type const &operator*() const;

    // increment/decrement
    OpaqueSeriesIterator &operator++();
    OpaqueSeriesIterator &operator--();
    OpaqueSeriesIterator operator++(int);
    OpaqueSeriesIterator operator--(int);

    // comparison
    bool operator==(OpaqueSeriesIterator const &other) const;
};

class AbstractSnapshotsContainer
{
public:
    using key_type = Iteration::IterationIndex_t;
    using value_type = Container<Iteration, key_type>::value_type;
    using mapped_type = Iteration;
    using iterator = OpaqueSeriesIterator<value_type>;
    using const_iterator = OpaqueSeriesIterator<value_type const>;
    // since AbstractSnapshotsContainer abstracts away the specific mode of
    // iteration, these are the same type
    using reverse_iterator = OpaqueSeriesIterator<value_type>;
    using const_reverse_iterator = OpaqueSeriesIterator<value_type const>;

    virtual auto currentIteration() -> std::optional<value_type *>;
    virtual auto currentIteration() const -> std::optional<value_type const *>;

    virtual auto begin() -> iterator = 0;
    virtual auto begin() const -> const_iterator = 0;
    virtual auto end() -> iterator = 0;
    virtual auto end() const -> const_iterator = 0;
    virtual auto rbegin() -> reverse_iterator = 0;
    virtual auto rbegin() const -> const_reverse_iterator = 0;
    virtual auto rend() -> reverse_iterator = 0;
    virtual auto rend() const -> const_reverse_iterator = 0;

    virtual auto empty() const -> bool = 0;
    virtual auto size() const -> size_t = 0;

    virtual auto at(key_type const &key) const -> mapped_type const & = 0;
    virtual auto at(key_type const &key) -> mapped_type &;

    virtual auto operator[](key_type const &key) -> mapped_type & = 0;

    virtual auto clear() -> void = 0;

    virtual auto find(key_type const &key) -> iterator = 0;
    virtual auto find(key_type const &key) const -> const_iterator = 0;

    virtual auto contains(key_type const &key) const -> bool = 0;
};
} // namespace openPMD
