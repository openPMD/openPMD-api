/* Copyright 2017-2021 Fabian Koller, Felix Schmitt, Axel Huebl
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

#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/config.hpp"

#include <hdf5.h>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>

namespace openPMD
{
struct GetH5DataType
{
    std::unordered_map<std::string, hid_t> m_userTypes;

    GetH5DataType(std::unordered_map<std::string, hid_t> userTypes)
        : m_userTypes{std::move(userTypes)}
    {}

    hid_t operator()(Attribute const &att);
};

hid_t getH5DataSpace(Attribute const &att);

std::string concrete_h5_file_position(Writable *w);

/** Computes the chunk dimensions for a dataset.
 *
 * Chunk dimensions are selected to create chunks sizes between
 * 64KByte and 4MB. Smaller chunk sizes are inefficient due to overhead,
 * larger chunks do not map well to file system blocks and striding.
 *
 * Chunk dimensions are less or equal to dataset dimensions and do
 * not need to be a factor of the respective dataset dimension.
 *
 * @param[in] dims dimensions of dataset to get chunk dims for
 * @param[in] typeSize size of each element in bytes
 * @return array for resulting chunk dimensions
 */
std::vector<hsize_t>
getOptimalChunkDims(std::vector<hsize_t> const dims, size_t const typeSize);
} // namespace openPMD
