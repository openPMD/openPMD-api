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
#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/RandomAccessIterator.hpp"
#include <functional>
#include <memory>
#include <optional>

namespace openPMD
{
class Snapshots
{
private:
    friend class Series;

    std::shared_ptr<AbstractSnapshotsContainer> m_snapshots;

    Snapshots(std::shared_ptr<AbstractSnapshotsContainer> snapshots);

    inline auto get() const -> AbstractSnapshotsContainer const &;
    inline auto get() -> AbstractSnapshotsContainer &;

public:
    using key_type = AbstractSnapshotsContainer::key_type;
    using value_type = AbstractSnapshotsContainer::value_type;
    using mapped_type = AbstractSnapshotsContainer::mapped_type;
    using iterator = AbstractSnapshotsContainer::iterator;
    using const_iterator = AbstractSnapshotsContainer::const_iterator;
    // since AbstractSnapshotsContainer abstracts away the specific mode of
    // iteration, these are the same type
    using reverse_iterator = AbstractSnapshotsContainer::reverse_iterator;
    using const_reverse_iterator =
        AbstractSnapshotsContainer::const_reverse_iterator;

    auto currentIteration() -> std::optional<value_type *>;
    auto currentIteration() const -> std::optional<value_type const *>;

    iterator begin();
    iterator end();
    const_iterator begin() const;
    const_iterator end() const;
    reverse_iterator rbegin();
    reverse_iterator rend();
    const_reverse_iterator rbegin() const;
    const_reverse_iterator rend() const;

    auto empty() const -> bool;
    auto size() const -> size_t;

    mapped_type const &at(key_type const &key) const;
    mapped_type &at(key_type const &key);

    mapped_type &operator[](key_type const &key);

    auto clear() -> void;

    // insert
    // swap

    auto find(key_type const &key) -> iterator;
    auto find(key_type const &key) const -> const_iterator;

    auto count(key_type const &key) const -> size_t;

    auto contains(key_type const &key) const -> bool;

    // erase
    // emplace
};

// backwards compatibility
using WriteIterations = Snapshots;
} // namespace openPMD
