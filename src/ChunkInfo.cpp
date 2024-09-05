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
#include "openPMD/ChunkInfo_internal.hpp"

#include "openPMD/auxiliary/Mpi.hpp"

#include <utility>

#ifdef _WIN32
#define openPMD_POSIX_AVAILABLE false
#else
#define openPMD_POSIX_AVAILABLE true
#endif

#if openPMD_POSIX_AVAILABLE
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
    constexpr size_t MAX_HOSTNAME_LENGTH = 256;

    Method methodFromStringDescription(
        std::string const &descr, [[maybe_unused]] bool consider_mpi)
    {
        static std::map<std::string, Method> const map{
            {"posix_hostname", Method::POSIX_HOSTNAME},
#if openPMD_HAVE_MPI
            {"hostname",
             consider_mpi ? Method::MPI_PROCESSOR_NAME
                          : Method::POSIX_HOSTNAME},
#else
            {"hostname", Method::POSIX_HOSTNAME},
#endif
            {"mpi_processor_name", Method::MPI_PROCESSOR_NAME}};
        return map.at(descr);
    }

    bool methodAvailable(Method method)
    {
        switch (method)
        {

        case Method::POSIX_HOSTNAME:
            return openPMD_POSIX_AVAILABLE;
        case Method::MPI_PROCESSOR_NAME:
            return openPMD_HAVE_MPI == 1;
        }
        throw std::runtime_error("Unreachable!");
    }

    std::string byMethod(Method method)
    {
        static std::map<Method, std::string (*)()> const map{
#if openPMD_POSIX_AVAILABLE
            {Method::POSIX_HOSTNAME, &posix_hostname},
#endif
#if openPMD_HAVE_MPI
            {Method::MPI_PROCESSOR_NAME, &mpi_processor_name},
#endif
        };
        try
        {
            return (*map.at(method))();
        }
        catch (std::out_of_range const &)
        {
            throw std::runtime_error(
                "[hostname::byMethod] Specified method is not available.");
        }
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

    std::string mpi_processor_name()
    {
        std::string res;
        res.resize(MPI_MAX_PROCESSOR_NAME);
        int string_len;
        if (MPI_Get_processor_name(res.data(), &string_len) != 0)
        {
            throw std::runtime_error(
                "[mpi_processor_name] Could not inquire processor name.");
        }
        // MPI_Get_processor_name returns the string length without null
        // terminator and std::string::resize() does not use null terminator
        // either. So, no +-1 necessary.
        res.resize(string_len);
        res.shrink_to_fit();
        return res;
    }
#endif

#if openPMD_POSIX_AVAILABLE
    std::string posix_hostname()
    {
        char hostname[MAX_HOSTNAME_LENGTH];
        if (gethostname(hostname, MAX_HOSTNAME_LENGTH))
        {
            throw std::runtime_error(
                "[posix_hostname] Could not inquire hostname.");
        }
        std::string res(hostname);
        return res;
    }
#endif
} // namespace host_info
} // namespace openPMD

#undef openPMD_POSIX_AVAILABLE
