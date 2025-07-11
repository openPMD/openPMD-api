#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/snapshots/IteratorTraits.hpp"
#include <optional>
#include <utility>

/* Public header due to use of AbstractSnapshotsContainer and its iterator type
 * OpaqueSeriesIterator in Snapshots class header. No direct user interaction
 * required with this header.
 */

namespace openPMD
{
/** Counterpart to Snapshots class:
 *  Iterator type that can wrap different implementations internally.
 */
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
    using parent_t =
        AbstractSeriesIterator<OpaqueSeriesIterator, value_type_in>;
    // no shared_ptr since copied iterators should not share state
    std::unique_ptr<DynamicSeriesIterator<value_type_in>> m_internal_iterator;

protected:
    using Self_t = OpaqueSeriesIterator<value_type_in>;

    template <typename ChildClass>
    auto to_concrete_iterator() -> std::optional<ChildClass>
    {
        auto bare_iterator =
            dynamic_cast<ChildClass *>(this->m_internal_iterator.get());
        if (!bare_iterator)
        {
            return std::nullopt;
        }
        return *bare_iterator;
    }

public:
    OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type_in>>
            internal_iterator);

    OpaqueSeriesIterator(OpaqueSeriesIterator const &other);
    OpaqueSeriesIterator(OpaqueSeriesIterator &&other) noexcept;
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator const &other);
    OpaqueSeriesIterator &operator=(OpaqueSeriesIterator &&other) noexcept;

    ~OpaqueSeriesIterator();

    // dereference
    using value_type = value_type_in;

    auto operator*() -> value_type &;
    auto operator*() const -> value_type const &;

    // increment/decrement
    OpaqueSeriesIterator &operator++();
    /** Not implemented for synchronous workflow:
     *  Reverse iteration not possible.
     */
    OpaqueSeriesIterator &operator--();
    /** Not implemented for synchronous workflow:
     *  Post increment not possible.
     */
    OpaqueSeriesIterator operator++(int);
    /** Not implemented for synchronous workflow:
     *  Reverse iteration not possible.
     */
    OpaqueSeriesIterator operator--(int);

    // comparison
    bool operator==(OpaqueSeriesIterator const &other) const;
};

/** Enum used as a label for distinguishing the different Snapshots
 * implementations.
 */
enum class SnapshotWorkflow
{
    RandomAccess,
    Synchronous
};

// Internal interface used by Snapshots class for interacting with containers.
// This needs to be in a public header since the type definition is used in
// private members of the Snapshots class which itself is a public class.
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
    using size_type = size_t;

    virtual ~AbstractSnapshotsContainer() = 0;

    virtual auto currentIteration() -> std::optional<value_type *>;
    virtual auto currentIteration() const
        -> std::optional<value_type const *> = 0;

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

    virtual auto erase(key_type const &key) -> size_type = 0;
    virtual auto erase(iterator) -> iterator = 0;

    virtual auto emplace(value_type &&) -> std::pair<iterator, bool> = 0;

    virtual auto snapshotWorkflow() const -> SnapshotWorkflow = 0;
};
} // namespace openPMD
