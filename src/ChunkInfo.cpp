/* Copyright 2020-2021 Franz Poeschel
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
#include "openPMD/ChunkInfo.hpp"

#include <utility>

namespace openPMD
{
ChunkInfo::ChunkInfo(Offset offset_in, Extent extent_in)
    : offset(std::move(offset_in)), extent(std::move(extent_in))
{}

bool ChunkInfo::operator==(ChunkInfo const &other) const
{
    return this->offset == other.offset && this->extent == other.extent;
}

WrittenChunkInfo::WrittenChunkInfo(
    Offset offset_in, Extent extent_in, int sourceID_in)
    : ChunkInfo(std::move(offset_in), std::move(extent_in))
    , sourceID(sourceID_in < 0 ? 0 : sourceID_in)
{}

WrittenChunkInfo::WrittenChunkInfo(Offset offset_in, Extent extent_in)
    : WrittenChunkInfo(std::move(offset_in), std::move(extent_in), 0)
{}

bool WrittenChunkInfo::operator==(WrittenChunkInfo const &other) const
{
    return this->sourceID == other.sourceID &&
        this->ChunkInfo::operator==(other);
}
} // namespace openPMD
