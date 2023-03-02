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

#include "openPMD/auxiliary/Mpi.hpp"

#include <utility>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

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

namespace host_info
{
    constexpr size_t MAX_HOSTNAME_LENGTH = 200;

    std::string byMethod(Method method)
    {
        static std::map<Method, std::string (*)()> map{
            {Method::HOSTNAME, &hostname}};
        return (*map[method])();
    }

#if openPMD_HAVE_MPI
    chunk_assignment::RankMeta byMethodCollective(MPI_Comm comm, Method method)
    {
        auto myHostname = byMethod(method);
        chunk_assignment::RankMeta res;
        auto allHostnames =
            auxiliary::distributeStringsToAllRanks(comm, myHostname);
        for (size_t i = 0; i < allHostnames.size(); ++i)
        {
            res[i] = allHostnames[i];
        }
        return res;
    }
#endif

    std::string hostname()
    {
        char hostname[MAX_HOSTNAME_LENGTH];
        if (gethostname(hostname, MAX_HOSTNAME_LENGTH))
        {
            throw std::runtime_error(
                "[gethostname] Could not inquire hostname.");
        }
        std::string res(hostname);
        return res;
    }
} // namespace host_info
} // namespace openPMD
