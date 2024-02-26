#pragma once

#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/RandomAccessIterator.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include <optional>

namespace openPMD
{
class StatefulSnapshotsContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    std::function<StatefulIterator *()> m_begin;
    StatefulSnapshotsContainer(std::function<StatefulIterator *()> begin);

    std::optional<StatefulIterator *> m_bufferedIterator = std::nullopt;

    auto get() -> StatefulIterator *;
    auto get() const -> StatefulIterator const *;

public:
    ~StatefulSnapshotsContainer() override;

    StatefulSnapshotsContainer(StatefulSnapshotsContainer const &other);
    StatefulSnapshotsContainer(StatefulSnapshotsContainer &&other) noexcept;

    StatefulSnapshotsContainer &
    operator=(StatefulSnapshotsContainer const &other);
    StatefulSnapshotsContainer &
    operator=(StatefulSnapshotsContainer &&other) noexcept;

    auto currentIteration() -> std::optional<value_type *> override;
    auto currentIteration() const -> std::optional<value_type const *> override;

    auto begin() -> iterator override;
    auto end() -> iterator override;
    auto begin() const -> const_iterator override;
    auto end() const -> const_iterator override;
    auto rbegin() -> iterator override;
    auto rend() -> iterator override;
    auto rbegin() const -> const_iterator override;
    auto rend() const -> const_iterator override;

    auto empty() const -> bool override;
    auto size() const -> size_t override;

    auto at(key_type const &key) const -> mapped_type const & override;
    auto at(key_type const &key) -> mapped_type & override;

    auto operator[](key_type const &key) -> mapped_type & override;

    auto clear() -> void override;

    auto find(key_type const &key) -> iterator override;
    auto find(key_type const &key) const -> const_iterator override;

    auto contains(key_type const &key) const -> bool override;
};

/*
 * @todo how to deal with iteration::open() iteration::close() ?
 * -> have it guaranteed with READ_LINEAR, not with READ_RANDOM_ACCESS,
 * but need to see how to deal with write modes
 */
class RandomAccessIteratorContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    Container<Iteration, key_type> m_cont;
    RandomAccessIteratorContainer(Container<Iteration, key_type> cont);

public:
    ~RandomAccessIteratorContainer() override;

    RandomAccessIteratorContainer(RandomAccessIteratorContainer const &other);
    RandomAccessIteratorContainer(
        RandomAccessIteratorContainer &&other) noexcept;

    RandomAccessIteratorContainer &
    operator=(RandomAccessIteratorContainer const &other);
    RandomAccessIteratorContainer &
    operator=(RandomAccessIteratorContainer &&other) noexcept;

    auto begin() -> iterator override;
    auto end() -> iterator override;
    auto begin() const -> const_iterator override;
    auto end() const -> const_iterator override;
    auto rbegin() -> iterator override;
    auto rend() -> iterator override;
    auto rbegin() const -> const_iterator override;
    auto rend() const -> const_iterator override;

    auto empty() const -> bool override;
    auto size() const -> size_t override;

    using AbstractSnapshotsContainer::at;
    auto at(key_type const &key) const -> mapped_type const & override;
    auto operator[](key_type const &key) -> mapped_type & override;

    auto clear() -> void override;

    auto find(key_type const &key) -> iterator override;
    auto find(key_type const &key) const -> const_iterator override;

    auto contains(key_type const &key) const -> bool override;
};
} // namespace openPMD
