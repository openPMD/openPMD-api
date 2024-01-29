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

    inline Snapshots(std::shared_ptr<AbstractSnapshotsContainer> snapshots)
        : m_snapshots(std::move(snapshots))
    {}

public:
    using key_type = AbstractSnapshotsContainer::key_type;
    using value_type = AbstractSnapshotsContainer::value_type;
    using iterator = AbstractSnapshotsContainer::iterator;
    using const_iterator = AbstractSnapshotsContainer::const_iterator;

    inline iterator begin()
    {
        return m_snapshots->begin();
    }
    inline iterator end()
    {
        return m_snapshots->end();
    }
    inline const_iterator begin() const
    {
        return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots)
            .begin();
    }
    inline const_iterator end() const
    {
        return static_cast<AbstractSnapshotsContainer const &>(*m_snapshots)
            .end();
    }
};
} // namespace openPMD
