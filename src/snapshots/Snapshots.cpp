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
#include <utility>

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/snapshots/Snapshots.hpp"
namespace openPMD
{
Snapshots::Snapshots(
    std::shared_ptr<AbstractSnapshotsContainer> snapshots,
    Attributable &iterations)
    : Attributable(Attributable::NoInit()), m_snapshots(std::move(snapshots))
{
    // Check the documentation of internal::AttributableData, we don't copy
    // the first-level pointer, only the second level
    // This avoids introducing depencencies between series.iterations and
    // series.snapshots()
    auto attributable_data = std::make_shared<internal::AttributableData>();
    attributable_data->cloneFrom(*iterations.m_attri);
    Attributable::setData(std::move(attributable_data));
}
inline auto Snapshots::get() const -> AbstractSnapshotsContainer const &
{
    return *m_snapshots;
}
inline auto Snapshots::get() -> AbstractSnapshotsContainer &
{
    return *m_snapshots;
}
auto Snapshots::currentIteration() -> std::optional<value_type *>
{
    return get().currentIteration();
}
auto Snapshots::currentIteration() const -> std::optional<value_type const *>
{
    return get().currentIteration();
}
auto Snapshots::begin() -> iterator
{
    return get().begin();
}
auto Snapshots::end() -> iterator
{
    return get().end();
}
auto Snapshots::begin() const -> const_iterator
{
    return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots)
        .begin();
}
auto Snapshots::end() const -> const_iterator
{
    return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots).end();
}
auto Snapshots::rbegin() -> reverse_iterator
{
    return get().begin();
}
auto Snapshots::rend() -> reverse_iterator
{
    return get().end();
}
auto Snapshots::rbegin() const -> const_reverse_iterator
{
    return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots)
        .begin();
}
auto Snapshots::rend() const -> const_reverse_iterator
{
    return get().end();
}

auto Snapshots::empty() const -> bool
{
    return get().empty();
}
auto Snapshots::size() const -> size_t
{
    return get().size();
}

auto Snapshots::at(key_type const &key) const -> mapped_type const &
{
    return get().at(key);
}
auto Snapshots::at(key_type const &key) -> mapped_type &
{
    return get().at(key);
}
auto Snapshots::operator[](key_type const &key) -> mapped_type &
{
    return get().operator[](key);
}

auto Snapshots::clear() -> void
{
    return get().clear();
}

auto Snapshots::find(key_type const &key) -> iterator
{
    return get().find(key);
}
auto Snapshots::find(key_type const &key) const -> const_iterator
{
    return get().find(key);
}

auto Snapshots::count(key_type const &key) const -> size_t
{
    return contains(key) ? 1 : 0;
}

auto Snapshots::contains(key_type const &key) const -> bool
{
    return get().contains(key);
}

auto Snapshots::erase(key_type const &key) -> size_type
{
    return get().erase(key);
}
auto Snapshots::erase(iterator it) -> iterator
{
    return get().erase(std::move(it));
}

auto Snapshots::snapshotWorkflow() const -> SnapshotWorkflow
{
    return get().snapshotWorkflow();
}

} // namespace openPMD
