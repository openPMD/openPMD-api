/* Copyright 2017-2021 Fabian Koller, Franz Poeschel, Axel Huebl
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

#include "openPMD/Datatype.hpp"

#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

namespace openPMD
{
using Extent = std::vector<std::uint64_t>;
using Offset = std::vector<std::uint64_t>;

class Dataset
{
    friend class RecordComponent;

public:
    enum : std::uint64_t
    {
        JOINED_DIMENSION = std::numeric_limits<std::uint64_t>::max()
    };

    Dataset(Datatype, Extent = {1}, std::string options = "{}");

    /**
     * @brief Constructor that sets the datatype to undefined.
     *
     * Helpful for resizing datasets, since datatypes need not be given twice.
     *
     */
    Dataset(Extent);

    Dataset &extend(Extent newExtent);

    Extent extent;
    Datatype dtype;
    uint8_t rank;
    std::string options = "{}"; //!< backend-dependent JSON configuration

    bool empty() const;

    std::optional<size_t> joinedDimension() const;
};
} // namespace openPMD
