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
#include <functional>
#include <memory>
#include <optional>

namespace openPMD
{
/** Entry point for accessing Snapshots/Iterations. Common API for different
 *  Snapshot accessing workflows.
 *
 * Refer to Series::snapshots() for a detailed description of different
 * workflows and their applications.
 * The API generally follows the Container/map conventions, though especially
 * the synchronous workflow does not implement all of the API calls.
 *
 * Since the synchronous workflow is tailored to support use cases from ADIOS2
 * which do not support going back to the start (IO steps, streaming), the
 * Container semantics are based on the remaining Iterations that have not yet
 * been seen. Especially does this mean that Snapshots::begin() does not reset
 * the Iterator back to the start, but will return the current state. There is
 * restricted support for going back to past Iterations by reopening them under
 * the condition that no previous ADIOS2 step needs to be activated. Note that
 * an Iteration handle goes invalid after closing it, a new Iteration handle is
 * acquired by Snapshots::operator[]().
 */
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

    /** @brief The currently active Iteration.
     *
     * Mostly useful for the synchronous workflow, since that has a notion of
     * global state where a specific Iteration is currently active (or no
     * Iteration, in which case a nullopt is returned). For random-access
     * workflow, the first Iteration in the Series is returned.
     */
    auto currentIteration() -> std::optional<value_type *>;
    auto currentIteration() const -> std::optional<value_type const *>;

    auto begin() -> iterator;
    auto end() -> iterator;
    /** Not implemented for synchronous workflow: Const iteration not possible.
     */
    auto begin() const -> const_iterator;
    /** Not implemented for synchronous workflow: Const iteration not possible.
     */
    auto end() const -> const_iterator;
    /** Not implemented for synchronous workflow:
     *  Reverse iteration not possible.
     */
    auto rbegin() -> reverse_iterator;
    /** Not implemented for synchronous workflow:
     *  Reverse iteration not possible.
     */
    auto rend() -> reverse_iterator;
    /** Not implemented for synchronous workflow:
     *  Const reverse iteration not possible.
     */
    auto rbegin() const -> const_reverse_iterator;
    /** Not implemented for synchronous workflow:
     *  Const reverse iteration not possible.
     */
    auto rend() const -> const_reverse_iterator;

    /** In synchronous workflow, this tells if there are remaining Iterations or
     *  not.
     */
    auto empty() const -> bool;
    /** Not implemented in synchronous workflow due to unclear semantics (past
     *  Iterations should not be considered, future Iterations are not yet
     *  known).
     */
    auto size() const -> size_t;

    /** Select an Iteration within the current IO step.
     */
    auto at(key_type const &key) const -> mapped_type const &;
    /** Select an Iteration within the current IO step.
     */
    auto at(key_type const &key) -> mapped_type &;

    auto operator[](key_type const &key) -> mapped_type &;

    /** Not implmented in synchronous workflow.
     */
    auto clear() -> void;

    // insert
    // swap

    /** Not implmented in synchronous workflow.
     *
     * (In future: Might be implemented in terms of searching within the current
     * IO step)
     */
    auto find(key_type const &key) -> iterator;
    /** Not implmented in synchronous workflow.
     *
     * (In future: Might be implemented in terms of checking the current
     * Iteration against the searched key and returning end() if it doesn't
     * match. Anything else is not possible since the shared Iterator would have
     * to be modified.)
     */
    auto find(key_type const &key) const -> const_iterator;

    /** Implemented in terms of contains(), see there.
     */
    auto count(key_type const &key) const -> size_t;

    /** Not implmented in synchronous workflow.
     */
    auto contains(key_type const &key) const -> bool;

    // erase
    // emplace
};

// backwards compatibility
using WriteIterations = Snapshots;
} // namespace openPMD
