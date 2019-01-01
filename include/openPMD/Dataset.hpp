/* Copyright 2017-2019 Fabian Koller
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

#include <memory>
#include <type_traits>
#include <vector>
#include <string>


namespace openPMD
{
using Extent = std::vector< std::uint64_t >;
using Offset = std::vector< std::uint64_t >;

class Dataset
{
    friend class RecordComponent;

public:
    Dataset(Datatype, Extent);

    Dataset& extend(Extent newExtent);
    Dataset& setChunkSize(Extent const&);
    Dataset& setCompression(std::string const&, uint8_t const);
    Dataset& setCustomTransform(std::string const&);

    Extent extent;
    Datatype dtype;
    uint8_t rank;
    Extent chunkSize;
    std::string compression;
    std::string transform;
};
} // openPMD
