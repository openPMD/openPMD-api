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
#include "openPMD/auxiliary/Mpi.hpp"

#include <algorithm>
#include <numeric>

#if openPMD_HAVE_MPI

namespace openPMD::auxiliary
{
StringMatrix collectStringsAsMatrixTo(
    MPI_Comm communicator, int destRank, std::string const &thisRankString)
{
    int rank, size;
    MPI_Comm_rank(communicator, &rank);
    MPI_Comm_size(communicator, &size);
    int sendLength = thisRankString.size() + 1;
    std::vector<int> recvcounts;

    if (rank == destRank)
    {
        recvcounts.resize(size);
    }

    MPI_Gather(
        &sendLength,
        1,
        MPI_INT,
        recvcounts.data(),
        1,
        MPI_INT,
        destRank,
        MPI_COMM_WORLD);
    int maxLength = std::accumulate(
        recvcounts.begin(), recvcounts.end(), 0, [](int a, int b) {
            return std::max(a, b);
        });

    StringMatrix res;
    std::vector<int> displs;
    if (rank == destRank)
    {
        res.line_length = maxLength;
        res.num_lines = size;
        res.char_buffer.resize(maxLength * res.num_lines);
        displs.reserve(size);
        for (int i = 0; i < size; ++i)
        {
            displs.emplace_back(i * maxLength);
        }
    }

    MPI_Gatherv(
        thisRankString.c_str(),
        sendLength,
        MPI_CHAR,
        res.char_buffer.data(),
        recvcounts.data(),
        displs.data(),
        MPI_CHAR,
        destRank,
        MPI_COMM_WORLD);

    return res;
}

std::vector<std::string> distributeStringsToAllRanks(
    MPI_Comm communicator, std::string const &thisRankString)
{
    int rank, size;
    MPI_Comm_rank(communicator, &rank);
    MPI_Comm_size(communicator, &size);
    int sendLength = thisRankString.size() + 1;

    int *sizesBuffer = new int[size];
    int *displs = new int[size];

    MPI_Allgather(
        &sendLength, 1, MPI_INT, sizesBuffer, 1, MPI_INT, MPI_COMM_WORLD);

    char *namesBuffer;
    {
        size_t sum = 0;
        for (int i = 0; i < size; ++i)
        {
            displs[i] = sum;
            sum += sizesBuffer[i];
        }
        namesBuffer = new char[sum];
    }

    MPI_Allgatherv(
        thisRankString.c_str(),
        sendLength,
        MPI_CHAR,
        namesBuffer,
        sizesBuffer,
        displs,
        MPI_CHAR,
        MPI_COMM_WORLD);

    std::vector<std::string> hostnames(size);
    for (int i = 0; i < size; ++i)
    {
        hostnames[i] = std::string(namesBuffer + displs[i]);
    }

    delete[] sizesBuffer;
    delete[] displs;
    delete[] namesBuffer;
    return hostnames;
}
} // namespace openPMD::auxiliary
#endif
