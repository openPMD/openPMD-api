/* Copyright 2018-2021 Franz Poeschel
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

#include "openPMD/config.hpp"
#if openPMD_HAVE_MPI

#include "openPMD/Datatype.hpp"
#include "openPMD/Series.hpp"

#include "string.h"
#include <map>
#include <mpi.h>
#include <tuple>
#include <vector>

namespace openPMD
{
/**
 * The report for a single benchmark produced by
 * <openPMD/benchmark/mpi/MPIBenchmark>.
 * @tparam Duration Datatype to be used for storing a time interval.
 */
template <typename Duration>
struct MPIBenchmarkReport
{
    MPI_Comm communicator;

    MPIBenchmarkReport(MPI_Comm);

    /**
     * Time needed for writing and reading per compression strategy and level.
     */
    std::map<
        std::tuple<
            int, // rank
            std::string, // compression
            uint8_t, // compression level
            std::string, // extension
            int, // thread size
            Datatype,
            typename decltype(Series::iterations)::key_type>,
        std::pair<Duration, Duration> >
        durations;

    enum Selector
    {
        RANK = 0,
        COMPRESSION,
        COMPRESSION_LEVEL,
        BACKEND,
        NRANKS,
        DTYPE,
        ITERATIONS
    };

    /**
     * Add results for a certain compression strategy and level.
     *
     * @param rootThread The MPI rank which will collect the data.
     * @param compression Compression strategy.
     * @param level Compression level
     * @param extension The openPMD filename extension.
     * @param threadSize The MPI size.
     * @param dt The openPMD datatype.
     * @param iterations The number of iterations per compression strategy.
     * @param report A pair of write and read time measurements.
     */
    void addReport(
        int rootThread,
        std::string compression,
        uint8_t level,
        std::string extension,
        int threadSize,
        Datatype dt,
        typename decltype(Series::iterations)::key_type iterations,
        std::pair<Duration, Duration> const &report);

    /** Retrieve the time measured for a certain compression strategy.
     *
     * @param rank Which MPI rank's duration results to retrieve.
     * @param compression Compression strategy.
     * @param level Compression level
     * @param extension The openPMD filename extension.
     * @param threadSize The MPI size.
     * @param dt The openPMD datatype.
     * @param iterations The number of iterations per compression strategy.
     * @return A pair of write and read time measurements.
     */
    std::pair<Duration, Duration> getReport(
        int rank,
        std::string compression,
        uint8_t level,
        std::string extension,
        int threadSize,
        Datatype dt,
        typename decltype(Series::iterations)::key_type iterations);

private:
    template <typename D, typename Dummy = D>
    struct MPIDatatype
    {};

    template <typename Dummy>
    struct MPIDatatype<char, Dummy>
    {
        MPI_Datatype dt = MPI_CHAR;
    };
    template <typename Dummy>
    struct MPIDatatype<unsigned char, Dummy>
    {
        MPI_Datatype dt = MPI_UNSIGNED_CHAR;
    };
    template <typename Dummy>
    struct MPIDatatype<short, Dummy>
    {
        MPI_Datatype dt = MPI_SHORT;
    };
    template <typename Dummy>
    struct MPIDatatype<int, Dummy>
    {
        MPI_Datatype dt = MPI_INT;
    };
    template <typename Dummy>
    struct MPIDatatype<long, Dummy>
    {
        MPI_Datatype dt = MPI_LONG;
    };
    template <typename Dummy>
    struct MPIDatatype<float, Dummy>
    {
        MPI_Datatype dt = MPI_FLOAT;
    };
    template <typename Dummy>
    struct MPIDatatype<double, Dummy>
    {
        MPI_Datatype dt = MPI_DOUBLE;
    };
    template <typename Dummy>
    struct MPIDatatype<unsigned short, Dummy>
    {
        MPI_Datatype dt = MPI_UNSIGNED_SHORT;
    };
    template <typename Dummy>
    struct MPIDatatype<unsigned int, Dummy>
    {
        MPI_Datatype dt = MPI_UNSIGNED;
    };
    template <typename Dummy>
    struct MPIDatatype<unsigned long, Dummy>
    {
        MPI_Datatype dt = MPI_UNSIGNED_LONG;
    };
    template <typename Dummy>
    struct MPIDatatype<long double, Dummy>
    {
        MPI_Datatype dt = MPI_LONG_DOUBLE;
    };
    template <typename Dummy>
    struct MPIDatatype<long long, Dummy>
    {
        MPI_Datatype dt = MPI_LONG_LONG_INT;
    };

    MPIDatatype<typename Duration::rep> m_mpiDatatype;
    MPI_Datatype mpiType = m_mpiDatatype.dt;
};

// implementation

template <typename Duration>
void MPIBenchmarkReport<Duration>::addReport(
    int rootThread,
    std::string compression,
    uint8_t level,
    std::string extension,
    int threadSize,
    Datatype dt,
    typename decltype(Series::iterations)::key_type iterations,
    std::pair<Duration, Duration> const &report)
{
    using rep = typename Duration::rep;
    // auto mpi_dt = MPIDatatype<rep>::dt;
    int rank;
    MPI_Comm_rank(communicator, &rank);
    int size;
    MPI_Comm_size(communicator, &size);
    MPI_Comm restricted;
    MPI_Comm_split(
        communicator, rank < threadSize ? 0 : MPI_UNDEFINED, rank, &restricted);
    rep readWrite[2];
    if (rank < threadSize)
    {
        readWrite[0] = report.first.count();
        readWrite[1] = report.second.count();
    }
    rep *recv = nullptr;
    if (rank == rootThread)
    {
        recv = new rep[2 * threadSize];
    }

    if (restricted != MPI_COMM_NULL)
    {
        MPI_Gather(
            readWrite,
            2, // should be 2 but doesnt work then..
            this->mpiType,
            recv,
            2,
            this->mpiType,
            rootThread,
            restricted);
    }

    if (rank == rootThread)
    {
        for (int i = 0; i < threadSize; i++)
        {
            Duration dWrite{recv[2 * i]};
            Duration dRead{recv[2 * i + 1]};
            this->durations.emplace(
                std::make_tuple(
                    i,
                    compression,
                    level,
                    extension,
                    threadSize,
                    dt,
                    iterations),
                std::make_pair(dWrite, dRead));
        }
        delete[] recv;
    }
    if (restricted != MPI_COMM_NULL)
    {
        MPI_Comm_free(&restricted);
    }
}

template <typename Duration>
MPIBenchmarkReport<Duration>::MPIBenchmarkReport(MPI_Comm comm)
    : communicator{comm}
{}

template <typename Duration>
std::pair<Duration, Duration> MPIBenchmarkReport<Duration>::getReport(
    int rank,
    std::string compression,
    uint8_t level,
    std::string extension,
    int threadSize,
    Datatype dt,
    typename decltype(Series::iterations)::key_type iterations)
{
    auto it = this->durations.find(std::make_tuple(
        rank, compression, level, extension, threadSize, dt, iterations));
    if (it == this->durations.end())
    {
        throw std::runtime_error(
            "Requested report not found. (Reports are available on the root "
            "thread only)");
    }
    else
    {
        return it->second;
    }
}

} // namespace openPMD

#endif
