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

#include "RandomDatasetFiller.hpp"

#include "openPMD/DatatypeHelpers.hpp"
#include "openPMD/benchmark/mpi/BlockSlicer.hpp"
#include "openPMD/benchmark/mpi/DatasetFiller.hpp"
#include "openPMD/benchmark/mpi/MPIBenchmarkReport.hpp"
#include "openPMD/openPMD.hpp"

#include <mpi.h>

#include <chrono>
#include <exception>
#include <iostream>
#include <set>
#include <sstream>
#include <tuple>
#include <utility>
#include <vector>

namespace openPMD
{
/**
 * Class representing a benchmark.
 * Allows to configure a benchmark and execute it.
 * @tparam DatasetFillerProvider Functor type to create a DatasetFiller with
 * the requested type. Should have a templated operator()() returning a value
 * that can be dynamically casted to a
 * std::shared_ptr<openPMD::DatasetFiller<T>>.
 */
template <typename DatasetFillerProvider>
class MPIBenchmark
{

public:
    using extentT = Extent::value_type;
    MPI_Comm communicator = MPI_COMM_WORLD;

    /**
     * Total extent of the hypercuboid used in the benchmark.
     */
    Extent totalExtent;

    std::shared_ptr<BlockSlicer> m_blockSlicer;

    DatasetFillerProvider m_dfp;

    /**
     * Construct an MPI benchmark manually.
     * @param basePath The path to write to. Will be extended with the
     * backends' filename endings. May be overwritten if performing several
     * benchmarks with the same backend, e.g. when using different compression
     * schemes.
     * @param tExtent The total extent of the dataset.
     * @param blockSlicer An implementation of BlockSlicer class, associating
     * each thread with a portion of the dataset to write to.
     * @param dfp DatasetFillerProvider, a templated functor returning a
     * std::shared_ptr<openPMD::DatasetFiller<T>> or a value dynamically
     * castable to one.
     * @param comm MPI communicator.
     */
    MPIBenchmark(
        std::string basePath,
        Extent tExtent,
        std::shared_ptr<BlockSlicer> blockSlicer,
        DatasetFillerProvider dfp,
        MPI_Comm comm = MPI_COMM_WORLD);

    /**
     * @param compression Compression string, leave empty to disable
     * commpression.
     * @param compressionLevel Compression level.
     * @param backend Backend to use, specified by filename extension (eg "bp"
     * or "h5").
     * @param dt Type of data to write and read.
     * @param iterations The number of iterations to write and read for each
     * compression strategy. The DatasetFiller functor will be called for each
     * iteration, so it should create sufficient data for one iteration.
     * @param threadSize Number of threads to use.
     */
    void addConfiguration(
        std::string compression,
        uint8_t compressionLevel,
        std::string backend,
        Datatype dt,
        typename decltype(Series::iterations)::key_type iterations,
        int threadSize);

    /**
     * Version of addConfiguration() that automatically sets the number of used
     * threads to the MPI size.
     * @param compression Compression string, leave empty to disable
     * commpression.
     * @param compressionLevel Compression level.
     * @param backend Backend to use, specified by filename extension (eg "bp"
     * or "h5").
     * @param dt Type of data to write and read.
     * @param iterations The number of iterations to write and read for each
     * compression strategy. The DatasetFiller functor will be called for each
     * iteration, so it should create sufficient data for one iteration.
     */
    void addConfiguration(
        std::string compression,
        uint8_t compressionLevel,
        std::string backend,
        Datatype dt,
        typename decltype(Series::iterations)::key_type iterations);

    void resetConfigurations();

    /**
     * Main function for running a benchmark. The benchmark is repeated for all
     * previously requested compressions strategies, backends and thread sizes.
     * @tparam Clock Clock type to use.
     * @param rootThread Rank at which the report will be read.
     * @return A report about the time needed for writing and reading under each
     * compression strategy.
     */
    template <typename Clock>
    MPIBenchmarkReport<typename Clock::duration>
    runBenchmark(int rootThread = 0);

private:
    std::string m_basePath;
    std::vector<std::tuple<
        std::string,
        uint8_t,
        std::string,
        int,
        Datatype,
        typename decltype(Series::iterations)::key_type>>
        m_configurations;

    enum Config
    {
        COMPRESSION = 0,
        COMPRESSION_LEVEL,
        BACKEND,
        NRANKS,
        DTYPE,
        ITERATIONS
    };

    std::pair<Offset, Extent> slice(int size);

    /**
     * @brief Struct used by MPIBenchmark::runBenchmark in switchType.
     *        Does the actual heavy lifting.
     *
     * @tparam Clock Clock type to use.
     */
    template <typename Clock>
    struct BenchmarkExecution
    {
        MPIBenchmark<DatasetFillerProvider> *m_benchmark;

        explicit BenchmarkExecution(
            MPIBenchmark<DatasetFillerProvider> *benchmark)
            : m_benchmark{benchmark}
        {}

        /**
         * Execute a single read benchmark.
         * @tparam T Type of the dataset to write.
         * @param compression Compression to use.
         * @param level Compression level to use.
         * @param offset Local offset of the chunk to write.
         * @param extent Local extent of the chunk to write.
         * @param extension File extension to control the openPMD backend.
         * @param datasetFiller The DatasetFiller to provide data for writing.
         * @param iterations The number of iterations to write.
         * @return The time passed.
         */
        template <typename T>
        typename Clock::duration writeBenchmark(
            std::string const &compression,
            uint8_t level,
            Offset &offset,
            Extent &extent,
            std::string const &extension,
            std::shared_ptr<DatasetFiller<T>> datasetFiller,
            typename decltype(Series::iterations)::key_type iterations);

        /**
         * Execute a single read benchmark.
         * @tparam T Type of the dataset to read.
         * @param offset Local offset of the chunk to read.
         * @param extent Local extent of the chunk to read.
         * @param extension File extension to control the openPMD backend.
         * @param iterations The number of iterations to read.
         * @return The time passed.
         */
        template <typename T>
        typename Clock::duration readBenchmark(
            Offset &offset,
            Extent &extent,
            std::string extension,
            typename decltype(Series::iterations)::key_type iterations);

        template <typename T>
        void operator()(
            MPIBenchmarkReport<typename Clock::duration> &report,
            int rootThread = 0);

        template <int n>
        void operator()(MPIBenchmarkReport<typename Clock::duration> &, int);
    };
};

// Implementation

template <typename DatasetFillerProvider>
template <typename Clock>
MPIBenchmarkReport<typename Clock::duration>
MPIBenchmark<DatasetFillerProvider>::runBenchmark(int rootThread)
{
    MPIBenchmarkReport<typename Clock::duration> res{this->communicator};
    BenchmarkExecution<Clock> exec{this};

    std::set<Datatype> datatypes;
    for (auto const &conf : m_configurations)
    {
        datatypes.insert(std::get<DTYPE>(conf));
    }
    for (Datatype dt : datatypes)
    {
        switchType(dt, exec, res, rootThread);
    }

    return res;
}

template <typename DatasetFillerProvider>
MPIBenchmark<DatasetFillerProvider>::MPIBenchmark(
    std::string basePath,
    Extent tExtent,
    std::shared_ptr<BlockSlicer> blockSlicer,
    DatasetFillerProvider dfp,
    MPI_Comm comm)
    : communicator{comm}
    , totalExtent{std::move(tExtent)}
    , m_blockSlicer{std::move(blockSlicer)}
    , m_dfp{dfp}
    , m_basePath{std::move(basePath)}
{
    if (m_blockSlicer == nullptr)
        throw std::runtime_error("Argument blockSlicer cannot be a nullptr!");
}

template <typename DatasetFillerProvider>
std::pair<Offset, Extent> MPIBenchmark<DatasetFillerProvider>::slice(int size)
{
    int actualSize;
    MPI_Comm_size(this->communicator, &actualSize);
    int rank;
    MPI_Comm_rank(this->communicator, &rank);
    size = std::min(size, actualSize);
    return m_blockSlicer->sliceBlock(totalExtent, size, rank);
}

template <typename DatasetFillerProvider>
void MPIBenchmark<DatasetFillerProvider>::addConfiguration(
    std::string compression,
    uint8_t compressionLevel,
    std::string backend,
    Datatype dt,
    typename decltype(Series::iterations)::key_type iterations,
    int threadSize)
{
    this->m_configurations.emplace_back(
        compression, compressionLevel, backend, threadSize, dt, iterations);
}

template <typename DatasetFillerProvider>
void MPIBenchmark<DatasetFillerProvider>::addConfiguration(
    std::string compression,
    uint8_t compressionLevel,
    std::string backend,
    Datatype dt,
    typename decltype(Series::iterations)::key_type iterations)
{
    int size;
    MPI_Comm_size(communicator, &size);
    addConfiguration(
        compression, compressionLevel, backend, dt, iterations, size);
}

template <typename DatasetFillerProvider>
void MPIBenchmark<DatasetFillerProvider>::resetConfigurations()
{
    this->m_compressions.clear();
}

template <typename DatasetFillerProvider>
template <typename Clock>
template <typename T>
typename Clock::duration
MPIBenchmark<DatasetFillerProvider>::BenchmarkExecution<Clock>::writeBenchmark(
    std::string const &compression,
    uint8_t level,
    Offset &offset,
    Extent &extent,
    std::string const &extension,
    std::shared_ptr<DatasetFiller<T>> datasetFiller,
    typename decltype(Series::iterations)::key_type iterations)
{
    MPI_Barrier(m_benchmark->communicator);
    auto start = Clock::now();

    // open file for writing
    Series series = Series(
        m_benchmark->m_basePath + "." + extension,
        Access::CREATE,
        m_benchmark->communicator);

    for (typename decltype(Series::iterations)::key_type i = 0; i < iterations;
         i++)
    {
        auto writeData = datasetFiller->produceData();

        MeshRecordComponent id =
            series.iterations[i].meshes["id"][MeshRecordComponent::SCALAR];

        Datatype datatype = determineDatatype(writeData);
        Dataset dataset = Dataset(datatype, m_benchmark->totalExtent);
        if (!compression.empty())
        {
            dataset.setCompression(compression, level);
        }

        id.resetDataset(dataset);

        series.flush();

        id.storeChunk<T>(writeData, offset, extent);
        series.flush();
    }

    MPI_Barrier(m_benchmark->communicator);
    auto end = Clock::now();

    // deduct the time needed for data generation
    for (typename decltype(Series::iterations)::key_type i = 0; i < iterations;
         i++)
    {
        datasetFiller->produceData();
    }
    auto deduct = Clock::now();

    return end - start - (deduct - end);
}

template <typename DatasetFillerProvider>
template <typename Clock>
template <typename T>
typename Clock::duration
MPIBenchmark<DatasetFillerProvider>::BenchmarkExecution<Clock>::readBenchmark(
    Offset &offset,
    Extent &extent,
    std::string extension,
    typename decltype(Series::iterations)::key_type iterations)
{
    MPI_Barrier(m_benchmark->communicator);
    // let every thread measure time
    auto start = Clock::now();

    Series series = Series(
        m_benchmark->m_basePath + "." + extension,
        Access::READ_ONLY,
        m_benchmark->communicator);

    for (typename decltype(Series::iterations)::key_type i = 0; i < iterations;
         i++)
    {
        MeshRecordComponent id =
            series.iterations[i].meshes["id"][MeshRecordComponent::SCALAR];

        auto chunk_data = id.loadChunk<T>(offset, extent);
        series.flush();
    }

    MPI_Barrier(m_benchmark->communicator);
    auto end = Clock::now();
    return end - start;
}

template <typename DatasetFillerProvider>
template <typename Clock>
template <typename T>
void MPIBenchmark<DatasetFillerProvider>::BenchmarkExecution<Clock>::operator()(
    MPIBenchmarkReport<typename Clock::duration> &report, int rootThread)
{
    Datatype dt = determineDatatype<T>();
    auto dsf = std::dynamic_pointer_cast<DatasetFiller<T>>(
        m_benchmark->m_dfp.template operator()<T>());
    for (auto const &config : m_benchmark->m_configurations)
    {
        std::string compression;
        uint8_t compressionLevel;
        std::string backend;
        int size;
        Datatype dt2;
        typename decltype(Series::iterations)::key_type iterations;
        std::tie(
            compression, compressionLevel, backend, size, dt2, iterations) =
            config;

        if (dt != dt2)
        {
            continue;
        }

        auto localCuboid = m_benchmark->slice(size);

        extentT blockSize = 1;
        for (auto ext : localCuboid.second)
        {
            blockSize *= ext;
        }
        dsf->setNumberOfItems(blockSize);

        auto writeTime = writeBenchmark<T>(
            compression,
            compressionLevel,
            localCuboid.first,
            localCuboid.second,
            backend,
            dsf,
            iterations);
        auto readTime = readBenchmark<T>(
            localCuboid.first, localCuboid.second, backend, iterations);
        report.addReport(
            rootThread,
            compression,
            compressionLevel,
            backend,
            size,
            dt2,
            iterations,
            std::make_pair(writeTime, readTime));
    }
}

template <typename DatasetFillerProvider>
template <typename Clock>
template <int n>
void MPIBenchmark<DatasetFillerProvider>::BenchmarkExecution<Clock>::operator()(
    MPIBenchmarkReport<typename Clock::duration> &, int)
{
    throw std::runtime_error(
        "Unknown/unsupported datatype requested to be benchmarked.");
}

} // namespace openPMD

#endif
