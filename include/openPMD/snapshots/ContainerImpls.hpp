/* Copyright 2025 Franz Poeschel
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

#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/RandomAccessIterator.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"

#include <optional>

/*
 * Private header, not included in user code.
 */

namespace openPMD
{
class StatefulSnapshotsContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;

    struct Members
    {
        /*
         * Consider the following user code:
         * > auto iterations = series.snapshots();
         * > for (auto & iteration : iterations) { ... }
         *
         * Here, only the for loop should actually wait for Iteration data. For
         * ensuring that Iterations are not waited for too early, the
         * initialization procedures are stored as a `std::function` in here and
         * only called upon the right moment. Until then, m_bufferedIterator
         * stays empty.
         * Compare the implementation of Series::snapshots(). In there, m_begin
         * is defined either by make_writing_stateful_iterator or
         * make_reading_stateful_iterator.
         * The iterator is resolved upon calling get() below.
         */

        // Need to put the deferred function behind a shared_ptr to avoid a
        // gcc14 compiler bug
        // warning: '*(std::_Function_base*)((char*)this +
        // 8).std::_Function_base::_M_manager' may be used uninitialized
        using Deferred_t = std::shared_ptr<std::function<StatefulIterator *()>>;
        using Evaluated_t = StatefulIterator *;
        using BufferedIterator_t = std::variant<Deferred_t, Evaluated_t>;
        BufferedIterator_t m_bufferedIterator = nullptr;
    };
    Members members;

    StatefulSnapshotsContainer(
        std::variant<std::function<StatefulIterator *()>, StatefulIterator *>
            begin);

    auto get() -> StatefulIterator *;
    auto get() const -> StatefulIterator const *;

    using value_type =
        Container<Iteration, Iteration::IterationIndex_t>::value_type;
    static auto stateful_to_opaque(StatefulIterator const &it)
        -> OpaqueSeriesIterator<value_type>;

public:
    ~StatefulSnapshotsContainer() override;

    StatefulSnapshotsContainer(StatefulSnapshotsContainer const &other);
    StatefulSnapshotsContainer(StatefulSnapshotsContainer &&other) noexcept(
        noexcept(Members(std::declval<Members &&>())));

    StatefulSnapshotsContainer &
    operator=(StatefulSnapshotsContainer const &other);
    StatefulSnapshotsContainer &
    operator=(StatefulSnapshotsContainer &&other) noexcept(noexcept(
        std::declval<Members>().operator=(std::declval<Members &&>())));

    using AbstractSnapshotsContainer::currentIteration;
    auto currentIteration() const -> std::optional<value_type const *> override;
    auto currentIteration() -> std::optional<value_type *> override;

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

    auto erase(key_type const &key) -> size_type override;
    auto erase(iterator) -> iterator override;

    auto emplace(value_type &&) -> std::pair<iterator, bool> override;

    auto snapshotWorkflow() const -> SnapshotWorkflow override;
};

class RandomAccessIteratorContainer : public AbstractSnapshotsContainer
{
private:
    friend class Series;
    using Container_t = Iterations;
    Container_t m_cont;
    RandomAccessIteratorContainer(Iterations cont);

    using concrete_iterator_type = RandomAccessIterator<Container_t::iterator>;
    using concrete_reverse_iterator_type =
        RandomAccessIterator<Container_t::reverse_iterator>;
    using concrete_const_iterator_type =
        RandomAccessIterator<Container_t::const_iterator>;
    using concrete_const_reverse_iterator_type =
        RandomAccessIterator<Container_t::const_reverse_iterator>;

public:
    ~RandomAccessIteratorContainer() override;

    RandomAccessIteratorContainer(RandomAccessIteratorContainer const &other);
    RandomAccessIteratorContainer(
        RandomAccessIteratorContainer &&other) noexcept;

    RandomAccessIteratorContainer &
    operator=(RandomAccessIteratorContainer const &other);
    RandomAccessIteratorContainer &
    operator=(RandomAccessIteratorContainer &&other) noexcept;

    using AbstractSnapshotsContainer::currentIteration;
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

    using AbstractSnapshotsContainer::at;
    auto at(key_type const &key) const -> mapped_type const & override;
    auto operator[](key_type const &key) -> mapped_type & override;

    auto clear() -> void override;

    auto find(key_type const &key) -> iterator override;
    auto find(key_type const &key) const -> const_iterator override;

    auto contains(key_type const &key) const -> bool override;

    auto erase(key_type const &key) -> size_type override;
    auto erase(iterator) -> iterator override;

    auto emplace(value_type &&) -> std::pair<iterator, bool> override;

    auto snapshotWorkflow() const -> SnapshotWorkflow override;
};
} // namespace openPMD
