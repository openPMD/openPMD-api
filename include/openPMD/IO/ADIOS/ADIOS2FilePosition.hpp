/* Copyright 2017-2021 Fabian Koller and Franz Poeschel
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

#include "openPMD/IO/AbstractFilePosition.hpp"
#include <string>
#include <utility>

namespace openPMD
{
struct ADIOS2FilePosition : public AbstractFilePosition
{
    enum class GD
    {
        GROUP,
        DATASET
    };

    ADIOS2FilePosition(std::string s, GD groupOrDataset)
        : location{std::move(s)}, gd{groupOrDataset}
    {}

    explicit ADIOS2FilePosition(GD groupOrDataset)
        : ADIOS2FilePosition{"/", groupOrDataset}
    {}

    ADIOS2FilePosition() : ADIOS2FilePosition{GD::GROUP}
    {}

    /**
     * Convention: Starts with slash '/', ends without.
     */
    std::string location;
    GD gd;
}; // ADIOS2FilePosition
} // namespace openPMD
