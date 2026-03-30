/* Copyright 2017-2025 Fabian Koller, Axel Huebl, Franz Poeschel, Junmin Gu,
 *                     Luca Fedeli
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
#include "openPMD/Series.hpp"
#include "openPMD/ChunkInfo.hpp"
#include "openPMD/ChunkInfo_internal.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/IO/DummyIOHandler.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/auxiliary/Date.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/auxiliary/Mpi.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Variant_internal.hpp"
#include "openPMD/snapshots/ContainerImpls.hpp"
#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/Snapshots.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include "openPMD/version.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace openPMD
{
namespace
{
    struct CleanedFilename
    {
        std::string body;
        std::string extension;

        std::tuple<std::string, std::string> decompose() &&
        {
            return std::tuple<std::string, std::string>{
                std::move(body), std::move(extension)};
        }
    };

    /** Remove the filename extension of a given storage format.
     *
     * @param   filename    String containing the filename, possibly with
     * filename extension.
     * @param   f           File format to remove filename extension for.
     * @return  String containing the filename without filename extension.
     */
    CleanedFilename cleanFilename(
        std::string const &filename, std::string const &filenameExtension);

    /** Compound return type for regex matching of filenames */
    struct Match
    {
        bool isContained{}; //! pattern match successful
        int padding{}; //! number of zeros used for padding of iteration, zero
                       //! if no padding
        Series::IterationIndex_t iteration =
            0; //! iteration found in regex pattern (default: 0)
        std::optional<std::string> extension;

        // support for std::tie
        operator std::tuple<bool &, int &, Series::IterationIndex_t &>()
        {
            return std::tuple<bool &, int &, Series::IterationIndex_t &>{
                isContained, padding, iteration};
        }
    };

    /** Create a functor to determine if a file can be of a format and matches
     * an iterationEncoding, given the filename on disk.
     *
     * @param   prefix      String containing head (i.e. before %T) of desired
     * filename without filename extension.
     * @param   padding     Amount of padding allowed in iteration number %T. If
     * zero, any amount of padding is matched.
     * @param   postfix     String containing tail (i.e. after %T) of desired
     * filename without filename extension.
     * @param   filenameExtension Filename extension to match against
     * @return  Functor returning tuple of bool and int.
     *          bool is True if file could be of type f and matches the
     * iterationEncoding. False otherwise. int is the amount of padding present
     * in the iteration number %T. Is 0 if bool is False.
     */
    std::function<Match(std::string const &)> matcher(
        std::string const &prefix,
        int padding,
        std::string const &postfix,
        std::optional<std::string> const &extension);

    struct TimeoutLazyParsing
    {
        using Clock = std::chrono::system_clock;
        Clock::time_point start_parsing, time_of_last_warning;
        uint64_t timeout;
        bool printed_warning_already = false;

        TimeoutLazyParsing(uint64_t timeout_in) : timeout(timeout_in)
        {
            if (timeout > 0)
            {
                start_parsing = Clock::now();
                time_of_last_warning = start_parsing;
            }
        }

        void now(size_t current_iteration_count, size_t total_iteration_count)
        {
            if (timeout == 0)
            {
                return;
            }
            auto current = Clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::seconds>(
                current - time_of_last_warning);
            if (uint64_t(diff.count()) >= timeout)
            {
                auto total_diff =
                    std::chrono::duration_cast<std::chrono::seconds>(
                        current - start_parsing);
                if (!printed_warning_already)
                {
                    std::cerr << &R"END(
[openPMD] WARNING: Parsing Iterations is taking a long time.
Consider using deferred Iteration parsing in order to open the Series lazily.
This can be achieved by either setting an environment variable:

> export OPENPMD_DEFER_ITERATION_PARSING=1

Or by specifying it as part of a JSON/TOML configuration:

> // C++:
> Series simData("my_data_%T.%E", R"({"defer_iteration_parsing": true})");
> // Python:
> simData = opmd.Series("my_data_%T.%E", {"defer_iteration_parsing": True})

Iterations will then be parsed only upon explicit user request:

> series.snapshots()[100].open()  // new API
> series.iterations[100].open()   // old API

Alternatively, Iterations will be opened implicitly when iterating in
READ_LINEAR access mode.
Refer also to the documentation at https://openpmd-api.readthedocs.io

This warning can be suppressed also by either specifying
an environment variable:

> export OPENPMD_HINT_LAZY_PARSING_TIMEOUT=0

Or by the JSON/TOML option {"hint_lazy_parsing_timeout": 0}.
)END"[1] << '\n';
                    printed_warning_already = true;
                }
                std::cerr << "Elapsed time: " << total_diff.count()
                          << "s, parsed " << current_iteration_count << " of "
                          << total_iteration_count << " Iterations."
                          << std::endl;
                time_of_last_warning = current;
            }
        }
    };
} // namespace

struct Series::ParsedInput
{
    std::string path;
    std::string name;
    Format format;
    IterationEncoding iterationEncoding;
    std::string filenamePrefix;
    std::string filenamePostfix;
    std::optional<std::string> filenameExtension;
    int filenamePadding = -1;
    // optional fields
    bool verify_homogeneous_extents = true;
}; // ParsedInput

std::string Series::openPMD() const
{
    return getAttribute("openPMD").get<std::string>();
}

Series &Series::setOpenPMD(std::string const &o)
{
    setAttribute("openPMD", o);
    auto standard = auxiliary::parseStandard(o);
    IOHandler()->m_standard = standard;
    if (standard >= OpenpmdStandard::v_2_0_0)
    {
        std::cerr << "[Warning] openPMD 2.0 is still under development.\n";
    }
    return *this;
}

uint32_t Series::openPMDextension() const
{
    return getAttribute("openPMDextension").get<uint32_t>();
}

Series &Series::setOpenPMDextension(uint32_t oe)
{
    setAttribute("openPMDextension", oe);
    return *this;
}

std::string Series::basePath() const
{
    return getAttribute("basePath").get<std::string>();
}

Series &Series::setBasePath(std::string const &bp)
{
    switch (IOHandler()->m_standard)
    {
    case OpenpmdStandard::v_1_0_0:
    case OpenpmdStandard::v_1_0_1:
    case OpenpmdStandard::v_1_1_0:
    case OpenpmdStandard::v_2_0_0:
        throw std::runtime_error(
            "Custom basePath not allowed in openPMD <=2.0");
    }

    setAttribute("basePath", bp);
    return *this;
}

std::string Series::meshesPath() const
{
    return getAttribute("meshesPath").get<std::string>();
}

Series &Series::setMeshesPath(std::string const &mp)
{
    auto &series = get();
    if (std::any_of(
            series.iterations.begin(),
            series.iterations.end(),
            [](Container<Iteration, IterationIndex_t>::value_type const &i) {
                return i.second.meshes.written();
            }))
        throw std::runtime_error(
            "A files meshesPath can not (yet) be changed after it has been "
            "written.");

    if (auxiliary::ends_with(mp, '/'))
        setAttribute("meshesPath", mp);
    else
        setAttribute("meshesPath", mp + "/");
    setDirty(true);
    return *this;
}

std::vector<std::string> Series::availableDatasets()
{
    if (iterationEncoding() == IterationEncoding::variableBased &&
        IOHandler()->m_backendAccess == Access::READ_RANDOM_ACCESS)
    {
        Parameter<Operation::ADVANCE> advance;
        advance.mode =
            Parameter<Operation::ADVANCE>::StepSelection{std::nullopt};
        IOHandler()->enqueue(IOTask(this, std::move(advance)));
    }
    Parameter<Operation::LIST_DATASETS> listDatasets;
    IOHandler()->enqueue(IOTask(this, listDatasets));
    IOHandler()->flush(internal::defaultFlushParams);
    return std::move(*listDatasets.datasets);
}

bool Series::hasRankTableRead()
{
    if (access::writeOnly(IOHandler()->m_frontendAccess))
    {
        return false;
    }
    auto &series = get();
    if (series.m_rankTable.m_bufferedRead.has_value())
    {
        return true;
    }
    auto datasets = availableDatasets();
    return std::find(datasets.begin(), datasets.end(), "rankTable") !=
        datasets.end();
}

#if openPMD_HAVE_MPI
chunk_assignment::RankMeta Series::rankTable(bool collective)
#else
chunk_assignment::RankMeta Series::rankTable([[maybe_unused]] bool collective)
#endif
{
    auto &series = get();
    auto &rankTable = series.m_rankTable;
    if (rankTable.m_bufferedRead.has_value())
    {
        return *rankTable.m_bufferedRead;
    }
    if (iterationEncoding() == IterationEncoding::fileBased)
    {
        std::cerr << "[Series] Use rank table in file-based iteration encoding "
                     "at your own risk. Make sure to have an iteration open "
                     "before calling this."
                  << std::endl;
        if (iterations.empty())
        {
            return {};
        }
#if 0
        Parameter<Operation::OPEN_FILE> openFile;
        openFile.name = iterationFilename(iterations.begin()->first);
        // @todo: check if the series currently has an open file, check if
        // collective is true
        IOHandler()->enqueue(IOTask(this, openFile));
#endif
    }
    auto datasets = availableDatasets();
    if (std::find(datasets.begin(), datasets.end(), "rankTable") ==
        datasets.end())
    {
        rankTable.m_bufferedRead = chunk_assignment::RankMeta{};
        return {};
    }
    Parameter<Operation::OPEN_DATASET> openDataset;
    openDataset.name = "rankTable";
    IOHandler()->enqueue(IOTask(&rankTable.m_attributable, openDataset));

    IOHandler()->flush(internal::defaultFlushParams);
    if (openDataset.extent->size() != 2)
    {
        // @todo use better error type
        throw std::runtime_error("[Series] rankTable must be 2D.");
    }
    if (*openDataset.dtype != Datatype::CHAR &&
        *openDataset.dtype != Datatype::UCHAR &&
        *openDataset.dtype != Datatype::SCHAR)
    {
        // @todo use better error type
        throw std::runtime_error("[Series] rankTable must have char type.");
    }

    auto writerRanks = (*openDataset.extent)[0];
    auto lineWidth = (*openDataset.extent)[1];

    if (lineWidth < 1)
    {
        // Check this because our indexing logic later relies on this
        // @todo use better error type
        throw std::runtime_error("[Series] rankTable lines must not be empty.");
    }

    std::shared_ptr<char> get{
        new char[writerRanks * lineWidth],
        [](char const *ptr) { delete[] ptr; }};

    auto doReadDataset = [&openDataset, this, &get, &rankTable]() {
        Parameter<Operation::READ_DATASET> readDataset;
        // read the whole thing
        readDataset.offset.resize(2);
        readDataset.extent = *openDataset.extent;
        // @todo better cross-platform support by switching over
        // *openDataset.dtype
        readDataset.dtype = Datatype::CHAR;
        readDataset.data = get;

        IOHandler()->enqueue(IOTask(&rankTable.m_attributable, readDataset));
        IOHandler()->flush(internal::defaultFlushParams);
    };

#if openPMD_HAVE_MPI
    if (collective && series.m_communicator.has_value())
    {
        auto comm = series.m_communicator.value();
        int rank{0}, size{1};
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        if (rank == 0)
        {
            doReadDataset();
        }
        MPI_Bcast(get.get(), writerRanks * lineWidth, MPI_CHAR, 0, comm);
    }
    else
    {
        doReadDataset();
    }
#else
    doReadDataset();
#endif

#if 0
    if (iterationEncoding() == IterationEncoding::fileBased)
    {
        // @todo only do this if the file was previously not open
        auto &it = iterations.begin()->second;
        Parameter<Operation::CLOSE_FILE> closeFile;
        IOHandler()->enqueue(IOTask(this, closeFile));
        it.get().m_closed = internal::CloseStatus::ClosedTemporarily;
        IOHandler()->flush(internal::defaultFlushParams);
    }
#endif

    chunk_assignment::RankMeta res;
    for (size_t i = 0; i < writerRanks; ++i)
    {
        if (get.get()[(i + 1) * lineWidth - 1] != 0)
        {
            throw std::runtime_error(
                "[Series] rankTable lines must be null-terminated strings.");
        }
        // Use C-String constructor for std::string in the following line
        // std::string::string(char const*);
        res[i] = get.get() + i * lineWidth;
    }
    rankTable.m_bufferedRead = res;
    return res;
}

Series &Series::setRankTable(const std::string &myRankInfo)
{
    get().m_rankTable.m_rankTableSource =
        internal::SeriesData::SourceSpecifiedManually{myRankInfo};
    return *this;
}

void Series::flushRankTable()
{
    auto &series = get();
    auto &rankTable = series.m_rankTable;
    auto maybeMyRankInfo = std::visit(
        auxiliary::overloaded{
            [](internal::SeriesData::NoSourceSpecified &)
                -> std::optional<std::string> { return std::nullopt; },
            [&series](internal::SeriesData::SourceSpecifiedViaJSON &viaJson)
                -> std::optional<std::string> {
                host_info::Method method;
                try
                {
#if openPMD_HAVE_MPI
                    bool consider_mpi = series.m_communicator.has_value();
#else
                    (void)series;
                    bool consider_mpi = false;
#endif
                    method = host_info::methodFromStringDescription(
                        viaJson.value, consider_mpi);
                }
                catch (std::out_of_range const &)
                {
                    throw error::WrongAPIUsage(
                        "[Series] Wrong value for JSON option 'rank_table': '" +
                        viaJson.value + "'.");
                }
                return host_info::byMethod(method);
            },
            [](internal::SeriesData::SourceSpecifiedManually &manually)
                -> std::optional<std::string> { return manually.value; }},
        rankTable.m_rankTableSource);
    if (!maybeMyRankInfo.has_value())
    {
        return;
    }

    auto myRankInfo = std::move(*maybeMyRankInfo);

    unsigned long long mySize = myRankInfo.size() + 1; // null character
    int rank{0}, size{1};
    unsigned long long maxSize = mySize;

    auto createRankTable = [&size, &maxSize, &rankTable, this]() {
        if (rankTable.m_attributable.written())
        {
            return;
        }
        Parameter<Operation::CREATE_DATASET> param(
            AbstractParameter::I_dont_want_to_use_joined_dimensions);
        param.name = "rankTable";
        param.dtype = Datatype::CHAR;
        param.extent = {uint64_t(size), uint64_t(maxSize)};
        IOHandler()->enqueue(
            IOTask(&rankTable.m_attributable, std::move(param)));
    };

    auto writeDataset = [&rank, &maxSize, this, &rankTable](
                            std::shared_ptr<char> put, size_t num_lines = 1) {
        Parameter<Operation::WRITE_DATASET> chunk;
        chunk.dtype = Datatype::CHAR;
        chunk.offset = {uint64_t(rank), 0};
        chunk.extent = {num_lines, maxSize};
        chunk.data = std::move(put);
        IOHandler()->enqueue(
            IOTask(&rankTable.m_attributable, std::move(chunk)));
    };

#if openPMD_HAVE_MPI
    if (series.m_communicator.has_value())
    {
        auto comm = *series.m_communicator;
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        // todo char portability
        auto [charBuffer, lineLength, numLines] =
            auxiliary::collectStringsAsMatrixTo(comm, 0, myRankInfo);
        (void)numLines; // it's the MPI size
        maxSize = lineLength;

        if (backend() == "MPI_HDF5")
        {
            MPI_Bcast(&maxSize, 1, MPI_UNSIGNED_LONG_LONG, 0, comm);
        }
        if (rank == 0 || backend() == "MPI_HDF5")
        {
            createRankTable();
        }

        if (rank == 0)
        {
            auto asRawPtr = new std::vector<char>(std::move(charBuffer));
            std::shared_ptr<char> put{
                asRawPtr->data(),
                /*
                 * A nicer solution would be to std::move() the vector into the
                 * closure and let RAII deal with it. But clang6 doesn't
                 * correctly implement C++17 closure move initialization, so
                 * we go the extra mile and use raw pointers.
                 * > [m_charBuffer = std::move(charBuffer)](char *){
                 * >     // no-op
                 * > }
                 */
                [asRawPtr](char *) { delete asRawPtr; }};
            writeDataset(std::move(put), /* num_lines = */ size);
        }

        // Must ensure that the Writable is consistently set to written on all
        // ranks
        series.m_rankTable.m_attributable.setWritten(
            true, EnqueueAsynchronously::OnlyAsync);
        return;
    }
#endif
    // sic! no else
    // if the Series was initialized without a communicator, then this code will
    // run as well
    createRankTable();

    std::shared_ptr<char> put{
        new char[maxSize]{}, [](char const *ptr) { delete[] ptr; }};
    std::copy_n(myRankInfo.c_str(), mySize, put.get());

    writeDataset(std::move(put));
}

std::string Series::particlesPath() const
{
    return getAttribute("particlesPath").get<std::string>();
}

Series &Series::setParticlesPath(std::string const &pp)
{
    auto &series = get();
    if (std::any_of(
            series.iterations.begin(),
            series.iterations.end(),
            [](Container<Iteration, IterationIndex_t>::value_type const &i) {
                return i.second.particles.written();
            }))
        throw std::runtime_error(
            "A files particlesPath can not (yet) be changed after it has been "
            "written.");

    if (auxiliary::ends_with(pp, '/'))
        setAttribute("particlesPath", pp);
    else
        setAttribute("particlesPath", pp + "/");
    setDirty(true);
    return *this;
}

std::string Series::author() const
{
    return getAttribute("author").get<std::string>();
}

Series &Series::setAuthor(std::string const &a)
{
    setAttribute("author", a);
    return *this;
}

std::string Series::software() const
{
    return getAttribute("software").get<std::string>();
}

Series &
Series::setSoftware(std::string const &newName, std::string const &newVersion)
{
    setAttribute("software", newName);
    setAttribute("softwareVersion", newVersion);
    return *this;
}

std::string Series::softwareVersion() const
{
    return getAttribute("softwareVersion").get<std::string>();
}

Series &Series::setSoftwareVersion(std::string const &sv)
{
    setAttribute("softwareVersion", sv);
    return *this;
}

std::string Series::date() const
{
    return getAttribute("date").get<std::string>();
}

Series &Series::setDate(std::string const &d)
{
    setAttribute("date", d);
    return *this;
}

std::string Series::softwareDependencies() const
{
    return getAttribute("softwareDependencies").get<std::string>();
}

Series &
Series::setSoftwareDependencies(std::string const &newSoftwareDependencies)
{
    setAttribute("softwareDependencies", newSoftwareDependencies);
    return *this;
}

std::string Series::machine() const
{
    return getAttribute("machine").get<std::string>();
}

Series &Series::setMachine(std::string const &newMachine)
{
    setAttribute("machine", newMachine);
    return *this;
}

IterationEncoding Series::iterationEncoding() const
{
    return get().m_iterationEncoding;
}

Series &Series::setIterationEncoding(IterationEncoding ie)
{
    setIterationEncoding_internal(ie, internal::default_or_explicit::explicit_);
    return *this;
}

std::string Series::iterationFormat() const
{
    return getAttribute("iterationFormat").get<std::string>();
}

Series &Series::setIterationFormat(std::string const &i)
{
    if (written())
        throw std::runtime_error(
            "A files iterationFormat can not (yet) be changed after it has "
            "been written.");

    if (iterationEncoding() == IterationEncoding::groupBased ||
        iterationEncoding() == IterationEncoding::variableBased)
    {
        if (!containsAttribute("basePath"))
        {
            setBasePath(i);
        }
        else if (
            basePath() != i &&
            IOHandler()->m_standard <= OpenpmdStandard::v_1_0_1)
            throw std::invalid_argument(
                "iterationFormat must not differ from basePath " + basePath() +
                " for group- or variableBased data");
    }

    setAttribute("iterationFormat", i);
    return *this;
}

std::string Series::name() const
{
    return get().m_name;
}

Series &Series::setName(std::string const &n)
{
    auto &series = get();
    if (series.m_deferred_initialization)
    {
        runDeferredInitialization();
    }
    if (written())
        throw std::runtime_error(
            "A files name can not (yet) be changed after it has been written.");

    if (series.m_iterationEncoding == IterationEncoding::fileBased)
    {
        // If the filename specifies an expansion pattern, set it.
        // If not, check if one is already active.
        // Our filename parser expects an extension, so just add any and ignore
        // the result for that
        if (hasExpansionPattern(n + ".json"))
        {
            reparseExpansionPattern(n + ".json");
        }
        else if (series.m_filenamePadding < 0)
        {
            throw error::WrongAPIUsage(
                "For fileBased formats the iteration expansion pattern %T must "
                "be included in the file name");
        }
        else
        {
            // no-op, keep the old pattern and set the new name
        }
    }

    series.m_name = n;
    setDirty(true);
    return *this;
}

std::string Series::backend() const
{
    return IOHandler()->backendName();
}

std::string Series::backend()
{
    /* this activates the non-const call to IOHandler() */
    return IOHandler()->backendName();
}

void Series::flush(std::string backendConfig)
{
    auto &series = get();
    flush_impl(
        series.iterations.begin(),
        series.iterations.end(),
        {FlushLevel::UserFlush, std::move(backendConfig)});
}

std::unique_ptr<Series::ParsedInput> Series::parseInput(std::string filepath)
{
    std::unique_ptr<Series::ParsedInput> input{new Series::ParsedInput};

#ifdef _WIN32
    if (auxiliary::contains(filepath, '/'))
    {
        std::cerr
            << "Filepaths on WINDOWS platforms may not contain slashes '/'! "
            << "Replacing with backslashes '\\' unconditionally!" << std::endl;
        filepath = auxiliary::replace_all(filepath, "/", "\\");
    }
#else
    if (auxiliary::contains(filepath, '\\'))
    {
        std::cerr
            << "Filepaths on UNIX platforms may not include backslashes '\\'! "
            << "Replacing with slashes '/' unconditionally!" << std::endl;
        filepath = auxiliary::replace_all(filepath, "\\", "/");
    }
#endif
    if (auxiliary::ends_with(filepath, auxiliary::directory_separator))
    {
        filepath = auxiliary::replace_last(
            filepath, std::string(&auxiliary::directory_separator, 1), "");
    }
    auto const pos = filepath.find_last_of(auxiliary::directory_separator);
    if (std::string::npos == pos)
    {
        input->path = ".";
        input->path.append(1, auxiliary::directory_separator);
        input->name = filepath;
    }
    else
    {
        input->path = filepath.substr(0, pos + 1);
        input->name = filepath.substr(pos + 1);
    }

    input->format = determineFormat(input->name);

    std::regex pattern("(.*)%(0[[:digit:]]+)?T(.*)");
    std::smatch regexMatch;
    std::regex_match(input->name, regexMatch, pattern);
    if (regexMatch.empty())
        input->iterationEncoding = IterationEncoding::groupBased;
    else if (regexMatch.size() == 4)
    {
        input->iterationEncoding = IterationEncoding::fileBased;
        input->filenamePrefix = regexMatch[1].str();
        std::string const &pad = regexMatch[2];
        if (pad.empty())
            input->filenamePadding = 0;
        else
        {
            if (pad.front() != '0')
                throw std::runtime_error(
                    "Invalid iterationEncoding " + input->name);
            input->filenamePadding = std::stoi(pad);
        }
        input->filenamePostfix = regexMatch[3].str();
    }
    else
        throw std::runtime_error(
            "Can not determine iterationFormat from filename " + input->name);

    input->filenamePostfix =
        cleanFilename(input->filenamePostfix, suffix(input->format)).body;

    std::tie(input->name, input->filenameExtension) =
        cleanFilename(input->name, suffix(input->format)).decompose();

    if (input->filenameExtension == ".%E")
    {
        input->filenameExtension = std::nullopt;
    }

    return input;
}

bool Series::hasExpansionPattern(std::string filenameWithExtension)
{
    auto input = parseInput(std::move(filenameWithExtension));
    return input->iterationEncoding == IterationEncoding::fileBased;
}

bool Series::reparseExpansionPattern(std::string filenameWithExtension)
{
    auto input = parseInput(std::move(filenameWithExtension));
    if (input->iterationEncoding != IterationEncoding::fileBased)
    {
        return false;
    }
    auto &series = get();
    series.m_filenamePrefix = input->filenamePrefix;
    series.m_filenamePostfix = input->filenamePostfix;
    series.m_filenamePadding = input->filenamePadding;
    return true;
}

namespace
{
    /*
     * Negative return values:
     * -1: No padding detected, just keep the default from the file name
     * -2: Contradicting paddings detected
     */
    template <typename MappingFunction>
    int autoDetectPadding(
        std::function<Match(std::string const &)> const &isPartOfSeries,
        std::string const &directory,
        MappingFunction &&mappingFunction)
    {
        std::set<int> paddings;
        if (auxiliary::directory_exists(directory))
        {
            for (auto const &entry : auxiliary::list_directory(directory))
            {
                Match match = isPartOfSeries(entry);
                if (match.isContained)
                {
                    paddings.insert(match.padding);
                    mappingFunction(entry, std::move(match));
                }
            }
        }
        if (paddings.size() == 1u)
            return *paddings.begin();
        else if (paddings.empty())
            return -1;
        else
            return -2;
    }

    int autoDetectPadding(
        std::function<Match(std::string const &)> const &isPartOfSeries,
        std::string const &directory)
    {
        return autoDetectPadding(isPartOfSeries, directory, [](auto &&...) {});
    }
} // namespace

template <typename... MPI_Communicator>
void Series::init(
    std::string const &filepath,
    Access at,
    std::string const &options,
    // Either an MPI_Comm or none, the template works for both options
    MPI_Communicator &&...comm)
{
    auto emplace_parse_config_options_into_iohandler =
        [](AbstractIOHandler &ioHandler, ParsedInput &input) {
            ioHandler.m_verify_homogeneous_extents =
                input.verify_homogeneous_extents;
        };

    auto init_directly = [this,
                          &comm...,
                          at,
                          &filepath,
                          &emplace_parse_config_options_into_iohandler](
                             std::unique_ptr<ParsedInput> parsed_input,
                             json::TracingJSON tracing_json) {
        auto io_handler = createIOHandler(
            std::nullopt,
            parsed_input->path,
            at,
            parsed_input->format,
            parsed_input->filenameExtension.value_or(std::string()),
            comm...,
            tracing_json,
            filepath);
        emplace_parse_config_options_into_iohandler(*io_handler, *parsed_input);
        initSeries(std::move(io_handler), std::move(parsed_input));
        json::warnGlobalUnusedOptions(tracing_json);
    };

    auto init_deferred = [this,
                          at,
                          &filepath,
                          &options,
                          &emplace_parse_config_options_into_iohandler,
                          &comm...](std::string const &parsed_directory) {
        // Set a temporary IOHandler so that API calls which require a present
        // IOHandler don't fail
        writable().IOHandler =
            std::make_shared<std::optional<std::unique_ptr<AbstractIOHandler>>>(
                std::make_unique<DummyIOHandler>(parsed_directory, at));
        auto &series = get();
        series.iterations.linkHierarchy(writable());
        series.m_rankTable.m_attributable.linkHierarchy(writable());
        series.m_deferred_initialization =
            [called_this_already = false,
             filepath,
             options,
             at,
             emplace_parse_config_options_into_iohandler,
             comm...](Series &s) mutable {
                if (called_this_already)
                {
                    throw std::runtime_error("Must be called one time only");
                }
                else
                {
                    called_this_already = true;
                }

                auto [parsed_input, tracing_json] =
                    s.initIOHandler<json::TracingJSON>(
                        filepath,
                        options,
                        at,
                        true,
                        std::forward<MPI_Communicator>(comm)...);

                auto &writable = s.get()->m_writable;

                auto io_handler = createIOHandler(
                    writable.IOHandler ? std::move(*writable.IOHandler)
                                       : std::nullopt,
                    parsed_input->path,
                    at,
                    parsed_input->format,
                    parsed_input->filenameExtension.value_or(std::string()),
                    comm...,
                    tracing_json,
                    filepath);
                emplace_parse_config_options_into_iohandler(
                    *io_handler, *parsed_input);
                auto res = io_handler.get();
                s.initSeries(std::move(io_handler), std::move(parsed_input));
                json::warnGlobalUnusedOptions(tracing_json);
                return res;
            };
    };

    switch (at)
    {
    case Access::CREATE_RANDOM_ACCESS:
    case Access::READ_WRITE:
    case Access::READ_ONLY: {
        auto [parsed_input, tracing_json] = initIOHandler<json::TracingJSON>(
            filepath,
            options,
            at,
            true,
            std::forward<MPI_Communicator>(comm)...);
        init_directly(std::move(parsed_input), std::move(tracing_json));
    }
    break;
    case Access::CREATE_LINEAR:
    case Access::READ_LINEAR:
    case Access::APPEND_RANDOM_ACCESS:
    case Access::APPEND_LINEAR: {
        auto [first_parsed_input, first_tracing_json] =
            initIOHandler<json::TracingJSON>(
                filepath,
                options,
                at,
                false,
                std::forward<MPI_Communicator>(comm)...);
        if (first_parsed_input->filenameExtension.has_value())
        {
            init_directly(
                std::move(first_parsed_input), std::move(first_tracing_json));
        }
        else
        {
            /*
             * Since we are still in the constructor, we want to avoid I/O
             * accesses to resolve the file extension at the moment.
             * -> Defer the proper initialization of the IO handler up to the
             * point when we actually need it.
             */
            init_deferred(first_parsed_input->path);
        }
    }
    break;
    }
}

template <typename TracingJSON, typename... MPI_Communicator>
auto Series::initIOHandler(
    std::string const &filepath,
    std::string const &options,
    Access at,
    bool resolve_generic_extension,
    MPI_Communicator &&...comm)
    -> std::tuple<std::unique_ptr<ParsedInput>, TracingJSON>
{
    auto &series = get();

    json::TracingJSON optionsJson = json::parseOptions(
        options,
        std::forward<MPI_Communicator>(comm)...,
        /* considerFiles = */ true);
    auto input = parseInput(filepath);
    if (resolve_generic_extension && input->format == Format::GENERIC &&
        !access::create(at))
    {
        auto isPartOfSeries =
            input->iterationEncoding == IterationEncoding::fileBased
            ? matcher(
                  input->filenamePrefix,
                  input->filenamePadding,
                  input->filenamePostfix,
                  std::nullopt)
            : matcher(input->name, -1, "", std::nullopt);
        std::optional<std::string> extension;
        std::set<std::string> additional_extensions;
        autoDetectPadding(
            isPartOfSeries,
            input->path,
            [&extension,
             &additional_extensions](std::string const &, Match const &match) {
                auto const &ext = match.extension.value();
                if (extension.has_value() && *extension != ext)
                {
                    additional_extensions.emplace(ext);
                }
                else
                {
                    extension = ext;
                }
            });
        if (extension.has_value())
        {
            if (!additional_extensions.empty())
            {
                std::stringstream error;
                error << "Found ambiguous filename extensions on disk: ";
                auto it = additional_extensions.begin();
                auto end = additional_extensions.end();
                error << '\'' << *it++ << '\'';
                for (; it != end; ++it)
                {
                    error << ", '" << *it << '\'';
                }
                error << " and '" + *extension + "'.";
                throw error::ReadError(
                    error::AffectedObject::File,
                    error::Reason::Other,
                    std::nullopt,
                    error.str());
            }
            input->filenameExtension = *extension;
            input->format = determineFormat(*extension);
        }
        else if (access::read(at))
        {
            throw error::ReadError(
                error::AffectedObject::File,
                error::Reason::NotFound,
                std::nullopt,
                "No file found that matches given pattern '" + filepath + "'.");
        }
    }

    // default options
    series.m_parseLazily = at == Access::READ_LINEAR;

    // now check for user-specified options
    parseJsonOptions(optionsJson, *input);

    if (resolve_generic_extension && !input->filenameExtension.has_value())
    {
        if (input->format == /* still */ Format::GENERIC)
        {
            throw error::WrongAPIUsage(
                "Unable to automatically determine filename extension. Please "
                "specify in some way.");
        }
        else if (input->format == Format::ADIOS2_BP)
        {
            // Since ADIOS2 has multiple extensions depending on the engine,
            // we need to pass this job on to the backend
            input->filenameExtension = ".%E";
        }
        else
        {
            input->filenameExtension = suffix(input->format);
        }
    }
    return std::make_tuple(std::move(input), std::move(optionsJson));
}

void Series::initSeries(
    std::unique_ptr<AbstractIOHandler> ioHandler,
    std::unique_ptr<Series::ParsedInput> input)
{
    auto &series = get();
    auto &writable = series->m_writable;

    /*
     * In access modes APPEND and READ_LINEAR, a dummy IO Handler might have
     * been emplaced. The real IO Handler (ioHandler) was created from this
     * intermediate handler, moving from it.
     * In that case, the pointer is still valid, it just points to an empty
     * optional at the moment. Reuse the pointer, so that any objects that
     * might have been initialized with the old pointer are still valid.
     */
    if (writable.IOHandler)
    {
        *writable.IOHandler =
            std::make_optional<std::unique_ptr<AbstractIOHandler>>(
                std::move(ioHandler));
    }
    else
    {
        writable.IOHandler =
            std::make_shared<std::optional<std::unique_ptr<AbstractIOHandler>>>(
                std::move(ioHandler));
    }

    series.iterations.linkHierarchy(writable);
    series.iterations.writable().ownKeyWithinParent = "data";
    series.m_rankTable.m_attributable.linkHierarchy(writable);

    series.m_name = input->name;

    series.m_format = input->format;

    series.m_filenamePrefix = input->filenamePrefix;
    series.m_filenamePostfix = input->filenamePostfix;
    series.m_filenamePadding = input->filenamePadding;
    if (!input->filenameExtension)
    {
        throw error::Internal(
            "Control flow error: filename extension has not been resolved.");
    }
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    series.m_filenameExtension = input->filenameExtension.value();

    if (series.m_iterationEncoding == IterationEncoding::fileBased &&
        !series.m_filenamePrefix.empty() &&
        std::isdigit(
            static_cast<unsigned char>(*series.m_filenamePrefix.rbegin())))
    {
        std::cerr << R"END(
[Warning] In file-based iteration encoding, it is strongly recommended to avoid
digits as the last characters of the filename prefix.
For instance, a robust pattern is to prepend the expansion pattern
of the filename with an underscore '_'.
Example: 'data_%T.json' or 'simOutput_%06T.h5'
Given file pattern: ')END"
                  << series.m_name << "'" << std::endl;
    }

    switch (IOHandler()->m_frontendAccess)
    {
    case Access::READ_LINEAR:
        // don't parse anything here
        // no data accessible before opening the first step
        // setIterationEncoding(input->iterationEncoding);
        series.m_iterationEncoding = input->iterationEncoding;
        break;
    case Access::READ_ONLY:
    case Access::READ_WRITE: {
        /* Allow creation of values in Containers and setting of Attributes
         * Would throw for Access::READ_ONLY */
        IOHandler()->m_seriesStatus = internal::SeriesStatus::Parsing;

        try
        {
            if (input->iterationEncoding == IterationEncoding::fileBased)
                readFileBased(
                    /* read_only_this_single_iteration = */ std::nullopt);
            else
                readGorVBased(
                    /* do_always_throw_errors = */ false,
                    /* init = */ true,
                    /* read_only_this_single_iteration = */ std::nullopt);

            if (series.iterations.empty())
            {
                /* Access::READ_WRITE can be used to create a new Series
                 * allow setting attributes in that case */
                setWritten(false, Attributable::EnqueueAsynchronously::No);

                initDefaults(series.m_iterationEncoding);
                setIterationEncoding_internal(
                    series.m_iterationEncoding,
                    series.m_iterationEncodingSetExplicitly);

                setWritten(true, Attributable::EnqueueAsynchronously::No);
            }
        }
        catch (...)
        {
            IOHandler()->m_seriesStatus = internal::SeriesStatus::Default;
            throw;
        }

        IOHandler()->m_seriesStatus = internal::SeriesStatus::Default;
        break;
    }
    case Access::CREATE_RANDOM_ACCESS:
    case Access::CREATE_LINEAR: {
        initDefaults(input->iterationEncoding);
        setIterationEncoding_internal(
            input->iterationEncoding, series.m_iterationEncodingSetExplicitly);
        break;
    }
    case Access::APPEND_RANDOM_ACCESS:
    case Access::APPEND_LINEAR: {
        initDefaults(input->iterationEncoding);
        setIterationEncoding_internal(
            input->iterationEncoding, series.m_iterationEncodingSetExplicitly);
        if (input->iterationEncoding != IterationEncoding::fileBased)
        {
            break;
        }
        int padding = autoDetectPadding(
            matcher(
                series.m_filenamePrefix,
                series.m_filenamePadding,
                series.m_filenamePostfix,
                /*
                 * This might still be ".%E" if the backend is ADIOS2 and no
                 * files are yet on disk.
                 * In that case, this will just not find anything.
                 */
                series.m_filenameExtension),
            IOHandler()->directory);
        switch (padding)
        {
        case -2:
            throw std::runtime_error(
                "Cannot write to a series with inconsistent iteration padding. "
                "Please specify '%0<N>T' or open as read-only.");
        case -1:
            /*
             * No matching iterations found. No problem, Append mode is also
             * fine for creating new datasets.
             */
            break;
        default:
            series.m_filenamePadding = padding;
            break;
        }
        break;
    }
    }
    IOHandler()->m_lastFlushSuccessful = true;
}

void Series::initDefaults(IterationEncoding ie, bool initAll)
{
    if (!containsAttribute("basePath"))
    {
        if (ie == IterationEncoding::variableBased)
        {
            setAttribute(
                "basePath", auxiliary::replace_first(BASEPATH, "/%T/", ""));
        }
        else
        {
            setAttribute("basePath", std::string(BASEPATH));
        }
    }
    if (!containsAttribute("openPMD"))
        setOpenPMD(getStandardDefault());
    /*
     * In Append mode, only init the rest of the defaults after checking that
     * the file does not yet exist to avoid overriding more than needed.
     * In file-based iteration encoding, files are always truncated in Append
     * mode (Append mode works on a per-iteration basis).
     */
    if (!initAll && access::append(IOHandler()->m_frontendAccess) &&
        ie != IterationEncoding::fileBased)
    {
        return;
    }
    if (!containsAttribute("openPMDextension"))
        setOpenPMDextension(0);
    if (!containsAttribute("date"))
        setDate(auxiliary::getDateString());
    if (!containsAttribute("software"))
        setSoftware("openPMD-api", getVersion());
}

std::future<void> Series::flush_impl(
    iterations_iterator begin,
    iterations_iterator end,
    internal::FlushParams const &flushParams,
    bool flushIOHandler)
{
    if (!written() && access::readOnly(IOHandler()->m_frontendAccess) &&
        IOHandler()->m_work.empty())
    {
        throw error::WrongAPIUsage(
            "Attempting to flush the Series in Read mode even though no "
            "operation has been performed yet. If this error occurs to you in "
            "Access::READ_LINEAR mode, then either try using "
            "Series::parseBase() or use Series::snapshots() / "
            "Series::readIterations() to access the first Iteration. If this "
            "occurs in Access::READ_ONLY, then this is likely a bug.");
    }
    IOHandler()->m_lastFlushSuccessful = true;
    try
    {
        switch (iterationEncoding())
        {
            using IE = IterationEncoding;
        case IE::fileBased:
            flushFileBased(begin, end, flushParams, flushIOHandler);
            break;
        case IE::groupBased:
        case IE::variableBased:
            flushGorVBased(begin, end, flushParams, flushIOHandler);
            break;
        }
        if (flushIOHandler)
        {
            IOHandler()->m_lastFlushSuccessful = true;
            return IOHandler()->flush(flushParams);
        }
        else
        {
            IOHandler()->m_lastFlushSuccessful = true;
            return {};
        }
    }
    catch (...)
    {
        auto handler = IOHandler();
        handler->m_lastFlushSuccessful = false;
        while (!handler->m_work.empty())
        {
            handler->m_work.pop();
        }
        throw;
    }
}

void Series::flushFileBased(
    iterations_iterator begin,
    iterations_iterator end,
    internal::FlushParams const &flushParams,
    bool flushIOHandler)
{
    auto &series = get();
    if (end == begin &&
        /*
         * At parsing time, this might happen since iterations might contain
         * errors and be deleted.
         */
        IOHandler()->m_seriesStatus != internal::SeriesStatus::Parsing &&
        /*
         * Iterations might have been present, but have been closed and deleted
         * from internal structures. In this case, previous flushes were
         * successful and the Series is now in written() state.
         */
        !written())
    {
        throw error::WrongAPIUsage(
            "fileBased output can not be written with no iterations.");
    }

    switch (IOHandler()->m_frontendAccess)
    {
    case Access::READ_ONLY:
    case Access::READ_LINEAR:
        for (auto it = begin; it != end; ++it)
        {
            // Phase 1
            switch (openIterationIfDirty(it->first, it->second))
            {
                using IO = IterationOpened;
            case IO::RemainsClosed:
                // we might need to proceed further if the close status is
                // ClosedInFrontend
                // hence no continue here
                // otherwise, we might forget to close files physically
                break;
            case IO::HasBeenOpened:
                // continue below
                it->second.flush(flushParams);
                break;
            }

            // Phase 2
            if (it->second.get().m_closed ==
                internal::CloseStatus::ClosedInFrontend)
            {
                Parameter<Operation::CLOSE_FILE> fClose;
                IOHandler()->enqueue(IOTask(&it->second, std::move(fClose)));
                it->second.get().m_closed = internal::CloseStatus::Closed;
            }
        }

        // Phase 3
        if (flushIOHandler)
        {
            IOHandler()->flush(flushParams);
        }
        break;
    case Access::READ_WRITE:
    case Access::CREATE_RANDOM_ACCESS:
    case Access::CREATE_LINEAR:
    case Access::APPEND_RANDOM_ACCESS:
    case Access::APPEND_LINEAR: {
        bool allDirty = dirty();
        for (auto it = begin; it != end; ++it)
        {
            // Phase 1
            switch (openIterationIfDirty(it->first, it->second))
            {
                using IO = IterationOpened;
            case IO::HasBeenOpened: {
                /* as there is only one series,
                 * emulate the file belonging to each iteration as not yet
                 * written, even if the iteration itself is already written
                 * (to ensure that the Series gets reassociated with the
                 * current iteration by the backend)
                 */
                this->setWritten(
                    false, Attributable::EnqueueAsynchronously::Both);
                series.iterations.setWritten(
                    false, Attributable::EnqueueAsynchronously::Both);

                setDirty(dirty() || it->second.dirty());
                std::string filename = iterationFilename(it->first);

                if (!it->second.written())
                {
                    series.m_currentlyActiveIterations.emplace(it->first);
                }

                it->second.flushFileBased(filename, it->first, flushParams);

                series.iterations.flush(
                    auxiliary::replace_first(basePath(), "%T/", ""),
                    flushParams);

                flushAttributes(flushParams);
                break;
            }
            case IO::RemainsClosed:
                break;
            }

            // Phase 2
            if (it->second.get().m_closed ==
                internal::CloseStatus::ClosedInFrontend)
            {
                Parameter<Operation::CLOSE_FILE> fClose;
                IOHandler()->enqueue(IOTask(&it->second, std::move(fClose)));
                it->second.get().m_closed = internal::CloseStatus::Closed;
            }
            /* reset the dirty bit for every iteration (i.e. file)
             * otherwise only the first iteration will have updates attributes
             */
            setDirty(allDirty);
        }
        setDirty(false);

        // Phase 3
        if (flushIOHandler)
        {
            IOHandler()->flush(flushParams);
        }
        break;
    }
    }
}

void Series::flushGorVBased(
    iterations_iterator begin,
    iterations_iterator end,
    internal::FlushParams const &flushParams,
    bool flushIOHandler)
{
    auto &series = get();
    if (access::readOnly(IOHandler()->m_frontendAccess))
    {
        for (auto it = begin; it != end; ++it)
        {
            // Phase 1
            switch (openIterationIfDirty(it->first, it->second))
            {
                using IO = IterationOpened;
            case IO::RemainsClosed:
                // we might need to proceed further if the close status is
                // ClosedInFrontend
                // hence no continue here
                break;
            case IO::HasBeenOpened:
                // continue below
                if (randomAccessSteps() && !series.m_snapshotToStep.empty())
                {
                    Parameter<Operation::ADVANCE> param;
                    param.mode = Parameter<Operation::ADVANCE>::StepSelection{
                        series.m_snapshotToStep.at(it->first)};
                    IOHandler()->enqueue(IOTask(this, std::move(param)));
                }
                it->second.flush(flushParams);
                break;
            }

            // Phase 2
            if (it->second.get().m_closed ==
                internal::CloseStatus::ClosedInFrontend)
            {
                // the iteration has no dedicated file in group-based mode
                it->second.get().m_closed = internal::CloseStatus::Closed;
            }
        }

        // Phase 3
        Parameter<Operation::TOUCH> touch;
        IOHandler()->enqueue(IOTask(&writable(), touch));
        if (flushIOHandler)
        {
            IOHandler()->flush(flushParams);
        }
    }
    else
    {
        if (!written())
        {
            if (access::append(IOHandler()->m_frontendAccess))
            {
                Parameter<Operation::CHECK_FILE> param;
                param.name = series.m_name;
                IOHandler()->enqueue(IOTask(this, param));
                IOHandler()->flush(internal::defaultFlushParams);
                switch (*param.fileExists)
                {
                    using FE = Parameter<Operation::CHECK_FILE>::FileExists;
                case FE::DontKnow:
                case FE::No:
                    initDefaults(iterationEncoding(), /* initAll = */ true);
                    break;
                case FE::Yes:
                    break;
                }
            }
            Parameter<Operation::CREATE_FILE> fCreate;
            fCreate.name = series.m_name;
            IOHandler()->enqueue(IOTask(this, fCreate));

            flushRankTable();
        }

        series.iterations.flush(
            auxiliary::replace_first(basePath(), "%T/", ""), flushParams);

        for (auto it = begin; it != end; ++it)
        {
            // Phase 1
            switch (openIterationIfDirty(it->first, it->second))
            {
                using IO = IterationOpened;
            case IO::HasBeenOpened:
                if (!it->second.written())
                {
                    it->second.parent() = getWritable(&series.iterations);
                    series.m_currentlyActiveIterations.emplace(it->first);
                }
                switch (iterationEncoding())
                {
                    using IE = IterationEncoding;
                case IE::groupBased:
                    it->second.flushGroupBased(it->first, flushParams);
                    break;
                case IE::variableBased:
                    it->second.flushVariableBased(it->first, flushParams);
                    break;
                default:
                    throw std::runtime_error(
                        "[Series] Internal control flow error");
                }
                break;
            case IO::RemainsClosed:
                break;
            }

            // Phase 2
            if (it->second.get().m_closed ==
                internal::CloseStatus::ClosedInFrontend)
            {
                // the iteration has no dedicated file in group-based mode
                it->second.get().m_closed = internal::CloseStatus::Closed;
            }
        }

        flushAttributes(flushParams);
        Parameter<Operation::TOUCH> touch;
        IOHandler()->enqueue(IOTask(&writable(), touch));
        if (flushIOHandler)
        {
            IOHandler()->flush(flushParams);
        }
    }
}

void Series::flushMeshesPath()
{
    Parameter<Operation::WRITE_ATT> aWrite;
    aWrite.name = "meshesPath";
    Attribute a = getAttribute("meshesPath");
    aWrite.m_resource = a.getAny();
    aWrite.dtype = a.dtype;
    IOHandler()->enqueue(IOTask(this, aWrite));
}

void Series::flushParticlesPath()
{
    Parameter<Operation::WRITE_ATT> aWrite;
    aWrite.name = "particlesPath";
    Attribute a = getAttribute("particlesPath");
    aWrite.m_resource = a.getAny();
    aWrite.dtype = a.dtype;
    IOHandler()->enqueue(IOTask(this, aWrite));
}

void Series::readFileBased(
    std::optional<IterationIndex_t> read_only_this_single_iteration)
{
    auto &series = get();
    Parameter<Operation::OPEN_FILE> fOpen;
    Parameter<Operation::READ_ATT> aRead;

    // Tell the backend that we are parsing file-based iteration encoding.
    // This especially means that READ_RANDOM_ACCESS will be used instead of
    // READ_LINEAR, as READ_LINEAR is implemented in the frontend for file-based
    // encoding. Don't set the iteration encoding in the frontend yet, will be
    // set after reading the iteration encoding attribute from the opened file.
    IOHandler()->setIterationEncoding(IterationEncoding::fileBased);

    if (!auxiliary::directory_exists(IOHandler()->directory))
        throw error::ReadError(
            error::AffectedObject::File,
            error::Reason::Inaccessible,
            {},
            "Supplied directory is not valid: " + IOHandler()->directory);

    auto isPartOfSeries = matcher(
        series.m_filenamePrefix,
        series.m_filenamePadding,
        series.m_filenamePostfix,
        series.m_filenameExtension);

    int padding = autoDetectPadding(
        isPartOfSeries,
        IOHandler()->directory,
        // foreach found file with `filename` and `index`:
        [&series, &read_only_this_single_iteration](
            std::string const &filename, Match const &match) {
            auto index = match.iteration;
            if (read_only_this_single_iteration.has_value() &&
                *read_only_this_single_iteration != index)
            {
                return;
            }
            Iteration &i = series.iterations[index];
            series.m_iterationFilenames[index] =
                cleanFilename(filename, series.m_filenameExtension).body;
            i.deferParseAccess({std::to_string(index), index, true});
        });

    if (series.iterations.empty())
    {
        /* Frontend access type might change during Series::read() to allow
         * parameter modification. Backend access type stays unchanged for the
         * lifetime of a Series. */
        if (access::readOnly(IOHandler()->m_backendAccess))
            throw error::ReadError(
                error::AffectedObject::File,
                error::Reason::Inaccessible,
                {},
                "No matching iterations found: " + name());
        else
            std::cerr << "No matching iterations found: " << name()
                      << std::endl;
    }

    /*
     * Return true if parsing was successful
     */
    auto readIterationEagerly =
        [&read_only_this_single_iteration](
            Iteration &iteration) -> std::optional<error::ReadError> {
        try
        {
            iteration.runDeferredParseAccess();
        }
        catch (error::ReadError const &err)
        {
            return err;
        }
        // If one specific iteration is requested, keep it open afterward.
        if (!read_only_this_single_iteration.has_value())
        {
            Parameter<Operation::CLOSE_FILE> fClose;
            iteration.IOHandler()->enqueue(IOTask(&iteration, fClose));
            iteration.IOHandler()->flush(internal::defaultFlushParams);
            auto &it_data = iteration.get();
            it_data.m_closed = internal::CloseStatus::Closed;
            it_data.allow_reopening_implicitly = true;
        }
        return {};
    };
    std::vector<decltype(Series::iterations)::key_type> unparseableIterations;
    if (series.m_parseLazily)
    {
        for (auto &iteration : series.iterations)
        {
            iteration.second.get().m_closed =
                internal::CloseStatus::ParseAccessDeferred;
        }
        // open the first iteration, just to parse Series attributes
        bool atLeastOneIterationSuccessful = false;
        std::optional<error::ReadError> forwardFirstError;
        for (auto &pair : series.iterations)
        {
            if (read_only_this_single_iteration.has_value() &&
                *read_only_this_single_iteration != pair.first)
            {
                continue;
            }
            if (auto error = readIterationEagerly(pair.second); error)
            {
                std::cerr << "Cannot read iteration '" << pair.first
                          << "' and will skip it due to read error:\n"
                          << error->what() << std::endl;
                unparseableIterations.push_back(pair.first);
                if (!forwardFirstError.has_value())
                {
                    forwardFirstError = std::move(error);
                }
            }
            else
            {
                atLeastOneIterationSuccessful = true;
                break;
            }
        }
        if (!atLeastOneIterationSuccessful)
        {
            if (forwardFirstError.has_value())
            {
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                auto &firstError = *forwardFirstError;
                firstError.description.append(
                    "\n[Note] Not a single iteration can be successfully "
                    "parsed (see above errors). Returning the first observed "
                    "error, for better recoverability in user code. Need to "
                    "access at least one iteration even in deferred parsing "
                    "mode in order to read global Series attributes.");
                throw firstError;
            }
            else
            {
                throw error::ReadError(
                    error::AffectedObject::Other,
                    error::Reason::Other,
                    {},
                    "Not a single iteration can be successfully parsed (see "
                    "above errors). Need to access at least one iteration even "
                    "in deferred parsing mode in order to read global Series "
                    "attributes.");
            }
        }
    }
    else
    {
        bool atLeastOneIterationSuccessful = false;
        std::optional<error::ReadError> forwardFirstError;

        TimeoutLazyParsing timeout(series.m_hintLazyParsingAfterTimeout);

        size_t read_iterations = 0;
        for (auto &iteration : series.iterations)
        {
            if (read_only_this_single_iteration.has_value() &&
                *read_only_this_single_iteration != iteration.first)
            {
                continue;
            }
            if (!read_only_this_single_iteration.has_value())
            {
                timeout.now(read_iterations, iterations.size());
            }
            if (auto error = readIterationEagerly(iteration.second); error)
            {
                std::cerr << "Cannot read iteration '" << iteration.first
                          << "' and will skip it due to read error:\n"
                          << error->what() << std::endl;
                unparseableIterations.push_back(iteration.first);
                if (!forwardFirstError.has_value())
                {
                    forwardFirstError = std::move(error);
                }
            }
            else
            {
                atLeastOneIterationSuccessful = true;
            }
            ++read_iterations;
        }
        if (!atLeastOneIterationSuccessful)
        {
            if (forwardFirstError.has_value())
            {
                auto &firstError = forwardFirstError.value();
                firstError.description.append(
                    "\n[Note] Not a single iteration can be successfully "
                    "parsed (see above errors). Returning the first observed "
                    "error, for better recoverability in user code.");
                throw firstError;
            }
            else
            {
                throw error::ReadError(
                    error::AffectedObject::Other,
                    error::Reason::Other,
                    {},
                    "Not a single iteration can be successfully parsed (see "
                    "above warnings).");
            }
        }
    }

    for (auto index : unparseableIterations)
    {
        series.iterations.container().erase(index);
    }

    if (padding > 0)
        series.m_filenamePadding = padding;

    /* Frontend access type might change during SeriesInterface::read() to allow
     parameter modification.
     * Backend access type stays unchanged for the lifetime of a Series.
       autoDetectPadding() announces contradicting paddings with return status
       -2. */
    if (padding == -2 && IOHandler()->m_backendAccess == Access::READ_WRITE)
        throw std::runtime_error(
            "Cannot write to a series with inconsistent iteration padding. "
            "Please specify '%0<N>T' or open as read-only.");
}

void Series::readOneIterationFileBased(std::string const &filePath)
{
    auto &series = get();

    IOHandler()->m_encoding = IterationEncoding::fileBased;
    // In this case, READ_LINEAR is implemented exclusively in the frontend
    if (IOHandler()->m_backendAccess == Access::READ_LINEAR)
    {
        IOHandler()->m_backendAccess = Access::READ_ONLY;
    }
    Parameter<Operation::OPEN_FILE> fOpen;
    Parameter<Operation::READ_ATT> aRead;

    fOpen.name = filePath;
    IOHandler()->enqueue(IOTask(this, fOpen));
    IOHandler()->flush(internal::defaultFlushParams);
    series.iterations.parent() = getWritable(this);

    readBase();

    aRead.name = "iterationEncoding";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    IterationEncoding encoding_out;
    if (isSame(*aRead.dtype, Datatype::STRING))
    {
        std::string encoding = Attribute(Attribute::from_any, *aRead.m_resource)
                                   .get<std::string>();
        if (encoding == "fileBased")
            encoding_out = IterationEncoding::fileBased;
        else if (encoding == "groupBased")
        {
            encoding_out = IterationEncoding::fileBased;
            std::cerr
                << "Series constructor called with iteration regex '%T' "
                   "suggests loading a time series with fileBased iteration "
                   "encoding. Loaded file is groupBased. Will ignore the "
                   "encoding stated in the file and continue treating this as "
                   "file-based. Depending on what data the opened files "
                   "actually contain, this might not yield correct results.\n";
        }
        else if (encoding == "variableBased")
        {
            /*
             * Unlike if the file were group-based, this one doesn't work
             * at all since the group paths are different.
             */
            throw error::ReadError(
                error::AffectedObject::Other,
                error::Reason::Other,
                {},
                "Series constructor called with iteration "
                "regex '%T' suggests loading a "
                "time series with fileBased iteration "
                "encoding. Loaded file is variableBased.");
        }
        else
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Unknown iterationEncoding: " + encoding);
        auto old_written = written();
        setWritten(false, Attributable::EnqueueAsynchronously::No);
        setIterationEncoding_internal(
            encoding_out, internal::default_or_explicit::explicit_);
        setWritten(old_written, Attributable::EnqueueAsynchronously::Both);
    }
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'iterationEncoding' (expected "
            "string, found " +
            datatypeToString(
                Attribute(Attribute::from_any, *aRead.m_resource).dtype) +
            ")");

    aRead.name = "iterationFormat";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (isSame(*aRead.dtype, Datatype::STRING))
    {
        setWritten(false, Attributable::EnqueueAsynchronously::No);
        setIterationFormat(Attribute(Attribute::from_any, *aRead.m_resource)
                               .get<std::string>());
        setWritten(true, Attributable::EnqueueAsynchronously::No);
    }
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'iterationFormat' (expected "
            "string, found " +
                datatypeToString(
                    Attribute(Attribute::from_any, *aRead.m_resource).dtype) +
                ")");

    Parameter<Operation::OPEN_PATH> pOpen;
    std::string version = openPMD();
    if (version == "1.0.0" || version == "1.0.1" || version == "1.1.0" ||
        version == "2.0.0")
        pOpen.path = auxiliary::replace_first(basePath(), "/%T/", "");
    else
        throw error::ReadError(
            error::AffectedObject::File,
            error::Reason::UnexpectedContent,
            std::nullopt,
            "Unknown openPMD version - " + version +
                ". Consider upgrading your installation of the openPMD-api "
                "(https://openpmd-api.readthedocs.io).");
    IOHandler()->enqueue(IOTask(&series.iterations, pOpen));

    readAttributes(ReadMode::IgnoreExisting);
    series.iterations.readAttributes(ReadMode::OverrideExisting);
}

namespace
{
    /*
     * This function is efficient if subtract is empty and inefficient
     * otherwise. Use only where an empty subtract vector is the
     * common case.
     */
    template <typename V1, typename V2>
    void vectorDifference(V1 &baseVector, V2 const &subtract)
    {
        for (auto const &elem : subtract)
        {
            for (auto it = baseVector.begin(); it != baseVector.end(); ++it)
            {
                if (*it == elem)
                {
                    baseVector.erase(it);
                    break;
                }
            }
        }
    }
} // namespace

auto Series::readGorVBased(
    bool do_always_throw_errors,
    bool do_init,
    std::optional<IterationIndex_t> read_only_this_single_iteration)
    -> std::vector<IterationIndex_t>
{
    auto &series = get();
    Parameter<Operation::OPEN_FILE> fOpen;
    fOpen.name = series.m_name;
    IOHandler()->enqueue(IOTask(this, fOpen));
    IOHandler()->flush(internal::defaultFlushParams);
    series.m_parsePreference = *fOpen.out_parsePreference;

    if (do_init)
    {
        readBase();

        using DT = Datatype;
        Parameter<Operation::READ_ATT> aRead;
        aRead.name = "iterationEncoding";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);
        if (*aRead.dtype == DT::STRING)
        {
            std::string encoding =
                Attribute(Attribute::from_any, *aRead.m_resource)
                    .get<std::string>();
            if (encoding == "groupBased")
                series.m_iterationEncoding = IterationEncoding::groupBased;
            else if (encoding == "variableBased")
            {
                series.m_iterationEncoding = IterationEncoding::variableBased;
                if (IOHandler()->m_frontendAccess == Access::READ_WRITE)
                {
                    // @todo hmm see what happens
                    throw error::WrongAPIUsage(R"(
The opened Series uses variable-based encoding, but is being accessed by
READ_WRITE mode which does not (yet) support variable-based encoding.
Please choose either Access::READ_LINEAR for reading or Access::APPEND for
creating new iterations.
                    )");
                }
            }
            else if (encoding == "fileBased")
            {
                series.m_iterationEncoding = IterationEncoding::fileBased;
                /*
                 * Opening a single file of a file-based Series is a valid
                 * workflow, warnings are not necessary here.
                 * Leaving the old warning as a comment, because we might want
                 * to add this back in again if we add some kind of verbosity
                 * level specification or logging.
                 */
                // std::cerr << "Series constructor called with explicit "
                //              "iteration suggests loading a "
                //           << "single file with groupBased iteration encoding.
                //           "
                //              "Loaded file is fileBased.\n";
                /*
                 * We'll want the openPMD API to continue series.m_name to open
                 * the file instead of piecing the name together via
                 * prefix-padding-postfix things.
                 */
                series.m_overrideFilebasedFilename = series.m_name;
            }
            else
                throw error::ReadError(
                    error::AffectedObject::Attribute,
                    error::Reason::UnexpectedContent,
                    {},
                    "Unknown iterationEncoding: " + encoding);
            setAttribute("iterationEncoding", encoding);
        }
        else
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Unexpected Attribute datatype for 'iterationEncoding' "
                "(expected string, found " +
                    datatypeToString(
                        Attribute(Attribute::from_any, *aRead.m_resource)
                            .dtype) +
                    ")");

        aRead.name = "iterationFormat";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);
        if (*aRead.dtype == DT::STRING)
        {
            setWritten(false, Attributable::EnqueueAsynchronously::No);
            setIterationFormat(Attribute(Attribute::from_any, *aRead.m_resource)
                                   .get<std::string>());
            setWritten(true, Attributable::EnqueueAsynchronously::No);
        }
        else
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Unexpected Attribute datatype for 'iterationFormat' (expected "
                "string, found " +
                    datatypeToString(
                        Attribute(Attribute::from_any, *aRead.m_resource)
                            .dtype) +
                    ")");
    }

    Parameter<Operation::OPEN_PATH> pOpen;
    std::string version = openPMD();
    if (version == "1.0.0" || version == "1.0.1" || version == "1.1.0" ||
        version == "2.0.0")
        pOpen.path = auxiliary::replace_first(basePath(), "/%T/", "");
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unknown openPMD version - " + version);
    IOHandler()->enqueue(IOTask(&series.iterations, pOpen));

    readAttributes(ReadMode::IgnoreExisting);

    /*
     * 'snapshot' changes over steps, so reread that.
     */
    internal::withRWAccess(IOHandler()->m_seriesStatus, [&series]() {
        series.iterations.readAttributes(ReadMode::OverrideExisting);
    });

    /* obtain all paths inside the basepath (i.e. all iterations) */
    Parameter<Operation::LIST_PATHS> pList;
    IOHandler()->enqueue(IOTask(&series.iterations, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    /*
     * Return error if one is caught.
     */
    auto readSingleIteration = [&series, &pOpen, this](
                                   IterationIndex_t index,
                                   std::string const &path,
                                   internal::BeginStep const &beginStep)
        -> std::optional<error::ReadError> {
        if (series.iterations.contains(index))
        {
            // maybe re-read
            auto &i = series.iterations.at(index);
            // i.written(): the iteration has already been parsed
            // reparsing is not needed
            if (i.written())
            {
                return {};
            }
            if (i.get().m_closed != internal::CloseStatus::ParseAccessDeferred)
            {
                pOpen.path = path;
                IOHandler()->enqueue(IOTask(&i, pOpen));
                // @todo catch stuff from here too
                internal::withRWAccess(
                    IOHandler()->m_seriesStatus,
                    [&i, &path]() { i.reread(path); });
            }
        }
        else
        {
            // parse for the first time, resp. delay the parsing process
            Iteration &i = series.iterations[index];
            i.deferParseAccess({path, index, false, beginStep});
            if (!series.m_parseLazily)
            {
                try
                {
                    i.runDeferredParseAccess();
                }
                catch (error::ReadError const &err)
                {
                    std::cerr << "Cannot read iteration '" << index
                              << "' and will skip it due to read error:\n"
                              << err.what() << std::endl;
                    series.iterations.container().erase(index);
                    return {err};
                }
                i.get().m_closed = internal::CloseStatus::Open;
            }
            else
            {
                i.get().m_closed = internal::CloseStatus::ParseAccessDeferred;
            }
        }
        return std::nullopt;
    };

    auto currentSteps = currentSnapshot();

    TimeoutLazyParsing timeout{
        series.m_parseLazily ? 0 : series.m_hintLazyParsingAfterTimeout};
    size_t parsed_iterations = 0;

    switch (iterationEncoding())
    {
    case IterationEncoding::groupBased:
    /*
     * Sic! This happens when a file-based Series is opened in group-based mode.
     */
    case IterationEncoding::fileBased: {
        std::vector<IterationIndex_t> unreadableIterations;
        std::vector<IterationIndex_t> readableIterations;
        readableIterations.reserve(pList.paths->size());
        for (auto const &it : *pList.paths)
        {
            IterationIndex_t index = std::stoull(it);
            if (read_only_this_single_iteration.has_value() &&
                index != *read_only_this_single_iteration)
            {
                continue;
            }
            if (!read_only_this_single_iteration.has_value())
            {
                timeout.now(parsed_iterations, pList.paths->size());
            }
            if (auto err = internal::withRWAccess(
                    IOHandler()->m_seriesStatus,
                    [&]() {
                        return readSingleIteration(
                            index,
                            it,
                            internal::BeginStepTypes::DontBeginStep{});
                    });
                err)
            {
                std::cerr << "Cannot read iteration " << index
                          << " and will skip it due to read error:\n"
                          << err.value().what() << std::endl;
                if (do_always_throw_errors)
                {
                    throw *err;
                }
                unreadableIterations.push_back(index);
            }
            else
            {
                readableIterations.push_back(index);
            }
            ++parsed_iterations;
        }
        if (currentSteps.has_value())
        {
            auto &vec = currentSteps.value();
            vectorDifference(vec, unreadableIterations);
            return vec;
        }
        else
        {
            return readableIterations;
        }
    }
    case IterationEncoding::variableBased: {
        if (!currentSteps.has_value() || currentSteps.value().empty())
        {
            if (!read_only_this_single_iteration.has_value())
            {
                Parameter<Operation::LIST_DATASETS> ld;
                Parameter<Operation::LIST_PATHS> lp;
                IOHandler()->enqueue(IOTask(&iterations, ld));
                IOHandler()->enqueue(IOTask(&iterations, lp));
                IOHandler()->flush(internal::defaultFlushParams);
                if (ld.datasets->empty() && lp.paths->empty())
                {
                    return {}; // no iterations, just global attributes
                }
                else
                {
                    // there is data, defaulting to calling this Iteration idx 0
                    // when no further info is available
                    currentSteps = std::vector<IterationIndex_t>{0};
                }
            }
            else
            {
                currentSteps = std::vector<IterationIndex_t>{
                    *read_only_this_single_iteration};
            }
        }
        else if (read_only_this_single_iteration.has_value())
        {
            if (std::find(
                    currentSteps->begin(),
                    currentSteps->end(),
                    *read_only_this_single_iteration) != currentSteps->end())
            {
                *currentSteps = {*read_only_this_single_iteration};
            }
            else
            {
                currentSteps->clear();
            }
        }

        for (auto it : *currentSteps)
        {
            if (!read_only_this_single_iteration.has_value())
            {
                timeout.now(parsed_iterations, pList.paths->size());
            }
            /*
             * Variable-based iteration encoding relies on steps, so parsing
             * must happen after opening the first step.
             */
            using namespace internal;
            BeginStep beginStep = randomAccessSteps()
                ? (series.m_snapshotToStep.empty()
                       ? BeginStepTypes::make<BeginStepTypes::DontBeginStep>()
                       : BeginStepTypes::make<
                             BeginStepTypes::BeginStepRandomAccess>(
                             series.m_snapshotToStep.at(it)))
                : BeginStepTypes::make<BeginStepTypes::BeginStepSequentially>();
            if (auto err = internal::withRWAccess(
                    IOHandler()->m_seriesStatus,
                    [&readSingleIteration, it, &beginStep]() {
                        return readSingleIteration(it, "", beginStep);
                    });
                err)
            {
                /*
                 * Cannot recover from errors in this place.
                 * If there is an error in the first iteration, the Series
                 * cannot be read in variable-based encoding. The read API will
                 * try to skip other iterations that have errors.
                 */
                throw *err;
            }
            ++parsed_iterations;
        }
        return *currentSteps;
    }
    }
    throw std::runtime_error("Unreachable!");
}

void Series::readBase()
{
    auto &series = get();
    Parameter<Operation::READ_ATT> aRead;

    aRead.name = "openPMD";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (auto val = Attribute(Attribute::from_any, *aRead.m_resource)
                       .getOptional<std::string>();
        val.has_value())
        setOpenPMD(val.value());
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'openPMD' (expected string, "
            "found " +
                datatypeToString(
                    Attribute(Attribute::from_any, *aRead.m_resource).dtype) +
                ")");

    aRead.name = "openPMDextension";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (auto val = Attribute(Attribute::from_any, *aRead.m_resource)
                       .getOptional<uint32_t>();
        val.has_value())
        setOpenPMDextension(val.value());
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'openPMDextension' (expected "
            "uint32, found " +
                datatypeToString(
                    Attribute(Attribute::from_any, *aRead.m_resource).dtype) +
                ")");

    aRead.name = "basePath";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (auto val = Attribute(Attribute::from_any, *aRead.m_resource)
                       .getOptional<std::string>();
        val.has_value())
    {
        if ( // might have been previously initialized in READ_LINEAR access
             // mode
            containsAttribute("basePath") &&
            getAttribute("basePath").get<std::string>() != val.value())
        {
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Value for 'basePath' ('" + val.value() +
                    "') does not match expected value '" +
                    getAttribute("basePath").get<std::string>() + "'.");
        }
        setAttribute("basePath", val.value());
    }
    else
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            "Unexpected Attribute datatype for 'basePath' (expected string, "
            "found " +
                datatypeToString(
                    Attribute(Attribute::from_any, *aRead.m_resource).dtype) +
                ")");

    Parameter<Operation::LIST_ATTS> aList;
    IOHandler()->enqueue(IOTask(this, aList));
    IOHandler()->flush(internal::defaultFlushParams);
    if (std::count(
            aList.attributes->begin(), aList.attributes->end(), "meshesPath") ==
        1)
    {
        aRead.name = "meshesPath";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);
        if (auto val = Attribute(Attribute::from_any, *aRead.m_resource)
                           .getOptional<std::string>();
            val.has_value())
        {
            /* allow setting the meshes path after completed IO */
            for (auto &it : series.iterations)
                it.second.meshes.setWritten(
                    false, Attributable::EnqueueAsynchronously::No);

            setMeshesPath(val.value());

            for (auto &it : series.iterations)
                it.second.meshes.setWritten(
                    true, Attributable::EnqueueAsynchronously::No);
        }
        else
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Unexpected Attribute datatype for 'meshesPath' (expected "
                "string, found " +
                    datatypeToString(
                        Attribute(Attribute::from_any, *aRead.m_resource)
                            .dtype) +
                    ")");
    }
    else
    {
        // Make sure that the meshesPath does not leak from one iteration into
        // the other in file-based iteration encoding
        Attributable::get().m_attributes.erase("meshesPath");
    }

    if (std::count(
            aList.attributes->begin(),
            aList.attributes->end(),
            "particlesPath") == 1)
    {
        aRead.name = "particlesPath";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);
        if (auto val = Attribute(Attribute::from_any, *aRead.m_resource)
                           .getOptional<std::string>();
            val.has_value())
        {
            /* allow setting the meshes path after completed IO */
            for (auto &it : series.iterations)
                it.second.particles.setWritten(
                    false, Attributable::EnqueueAsynchronously::No);

            setParticlesPath(val.value());

            for (auto &it : series.iterations)
                it.second.particles.setWritten(
                    true, Attributable::EnqueueAsynchronously::No);
        }
        else
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Unexpected Attribute datatype for 'particlesPath' (expected "
                "string, found " +
                    datatypeToString(
                        Attribute(Attribute::from_any, *aRead.m_resource)
                            .dtype) +
                    ")");
    }
    {
        // Make sure that the particlesPath does not leak from one iteration
        // into the other in file-based iteration encoding
        Attributable::get().m_attributes.erase("particlesPath");
    }
}

std::string Series::iterationFilename(IterationIndex_t i)
{
    /*
     * The filename might have been overridden at the Series level or at the
     * Iteration level. See the struct members' documentation for the reasons.
     */
    auto &series = get();
    if (series.m_overrideFilebasedFilename.has_value())
    {
        return series.m_overrideFilebasedFilename.value();
    }
    else if (
        auto iteration = series.m_iterationFilenames.find(i); //
        iteration != series.m_iterationFilenames.end())
    {
        return iteration->second;
    }
    else
    {

        /*
         * If no filename has been explicitly stored, we use the filename
         * pattern to compute it.
         */
        std::stringstream iterationNr("");
        iterationNr << std::setw(series.m_filenamePadding) << std::setfill('0')
                    << i;
        return series.m_filenamePrefix + iterationNr.str() +
            series.m_filenamePostfix;
    }
}

Series::iterations_iterator Series::indexOf(Iteration const &iteration)
{
    auto &series = get();
    // first try the cached index; if it points to the correct entry return it
    auto idx = iteration.get().m_iterationIndex;
    if (!idx.has_value())
    {
        throw error::Internal("Iteration index not known.");
    }

    auto it = series.iterations.find(*idx);
    if (it != series.iterations.end() &&
        &it->second.Attributable::get() == &iteration.Attributable::get())
    {
        return it;
    }
    else
    {
        throw error::Internal(
            "Iteration " + std::to_string(*idx) +
            " no longer known by the Series?");
    }
}

AdvanceStatus Series::advance(
    AdvanceMode mode,
    internal::AttributableData &file,
    iterations_iterator begin)
{
    internal::FlushParams const flushParams = {FlushLevel::UserFlush};
    auto &series = get();
    auto end = begin;
    ++end;

    auto &iteration = begin->second;
    /*
     * We call flush_impl() with flushIOHandler = false, meaning that tasks are
     * not yet propagated to the backend.
     * We will append ADVANCE and CLOSE_FILE tasks manually and then flush the
     * IOHandler manually.
     * In order to avoid having those tasks automatically appended by
     * flush_impl(), set CloseStatus to Open for now.
     */
    auto &itData = iteration.get();
    internal::CloseStatus oldCloseStatus = itData.m_closed;
    if (oldCloseStatus == internal::CloseStatus::ClosedInFrontend)
    {
        itData.m_closed = internal::CloseStatus::Open;
    }

    bool old_dirty = iteration.dirty();
    iteration.setDirty(true); // force flush() to open this

    switch (mode)
    {
    case AdvanceMode::ENDSTEP:
        flush_impl(begin, end, flushParams, /* flushIOHandler = */ false);
        break;
    case AdvanceMode::BEGINSTEP:
        /*
         * When beginning a step, there is nothing to flush yet.
         * Data is not written in between steps.
         * So only make sure that files are accessed.
         */
        flush_impl(
            begin,
            end,
            {FlushLevel::CreateOrOpenFiles},
            /* flushIOHandler = */ false);
        iteration.setDirty(old_dirty);
        break;
    }

    if (oldCloseStatus == internal::CloseStatus::ClosedInFrontend)
    {
        // Series::flush() would normally turn a `ClosedInFrontend` into
        // a `ClosedInBackend`. Do that manually.
        itData.m_closed = internal::CloseStatus::Closed;
    }
    else if (
        oldCloseStatus == internal::CloseStatus::Closed &&
        series.m_iterationEncoding == IterationEncoding::fileBased)
    {
        /*
         * In file-based iteration encoding, we want to avoid accidentally
         * opening an iteration's file by beginning a step on it.
         * So, return now.
         */
        iteration.get().m_closed = internal::CloseStatus::Closed;
        return AdvanceStatus::OK;
    }

    if (mode == AdvanceMode::ENDSTEP)
    {
        flushStep(/* doFlush = */ false);
    }

    Parameter<Operation::ADVANCE> param;
    if (itData.m_closed == internal::CloseStatus::Closed &&
        series.m_iterationEncoding == IterationEncoding::fileBased)
    {
        /*
         * If the Series has file-based iteration layout and the file has not
         * been opened by flushFileFileBased(), there's no use in nagging the
         * backend to do anything.
         */
        param.status = std::make_shared<AdvanceStatus>(AdvanceStatus::OK);
    }
    else
    {
        param.mode = mode;
        if (iterationEncoding() == IterationEncoding::variableBased &&
            access::write(IOHandler()->m_frontendAccess) &&
            mode == AdvanceMode::BEGINSTEP && series.m_wroteAtLeastOneIOStep)
        {
            // If the backend does not support steps, we cannot continue here
            param.isThisStepMandatory = true;
        }
        IOTask task(&file->m_writable, param);
        IOHandler()->enqueue(task);
    }

    if (oldCloseStatus == internal::CloseStatus::ClosedInFrontend &&
        mode == AdvanceMode::ENDSTEP)
    {
        using IE = IterationEncoding;
        switch (series.m_iterationEncoding)
        {
        case IE::fileBased: {
            Parameter<Operation::CLOSE_FILE> fClose;
            IOHandler()->enqueue(IOTask(&iteration, std::move(fClose)));
            itData.m_closed = internal::CloseStatus::Closed;
            break;
        }
        case IE::groupBased: {
            // We can now put some groups to rest
            Parameter<Operation::CLOSE_PATH> fClose;
            IOHandler()->enqueue(IOTask(&iteration, std::move(fClose)));
            // In group-based iteration layout, files are
            // not closed on a per-iteration basis
            // We will treat it as such nonetheless
            itData.m_closed = internal::CloseStatus::Closed;
            break;
        }
        case IE::variableBased: // no action necessary
            break;
        }
    }

    // We cannot call Series::flush now, since the IO handler is still filled
    // from calling flush(Group|File)based, but has not been emptied yet
    // Do that manually
    IOHandler()->flush(flushParams);

    return *param.status;
}

AdvanceStatus Series::advance(AdvanceMode mode)
{
    auto &series = get();
    if (series.m_iterationEncoding == IterationEncoding::fileBased)
    {
        throw error::Internal(
            "Advancing a step in file-based iteration encoding is "
            "iteration-specific.");
    }
    internal::FlushParams const flushParams = {FlushLevel::UserFlush};
    /*
     * We call flush_impl() with flushIOHandler = false, meaning that tasks are
     * not yet propagated to the backend.
     * We will append ADVANCE and CLOSE_FILE tasks manually and then flush the
     * IOHandler manually.
     * In order to avoid having those tasks automatically appended by
     * flush_impl(), set CloseStatus to Open for now.
     */

    auto begin = iterations.end();
    auto end = iterations.end();

    switch (mode)
    {
    case AdvanceMode::ENDSTEP:
        flush_impl(begin, end, flushParams, /* flushIOHandler = */ false);
        break;
    case AdvanceMode::BEGINSTEP:
        /*
         * When beginning a step, there is nothing to flush yet.
         * Data is not written in between steps.
         * So only make sure that files are accessed.
         */
        flush_impl(
            begin,
            end,
            {FlushLevel::CreateOrOpenFiles},
            /* flushIOHandler = */ false);
        break;
    }

    if (mode == AdvanceMode::ENDSTEP)
    {
        flushStep(/* doFlush = */ false);
    }

    Parameter<Operation::ADVANCE> param;
    param.mode = mode;
    if (iterationEncoding() == IterationEncoding::variableBased &&
        access::write(IOHandler()->m_frontendAccess) &&
        mode == AdvanceMode::BEGINSTEP && series.m_wroteAtLeastOneIOStep)
    {
        // If the backend does not support steps, we cannot continue here
        param.isThisStepMandatory = true;
    }
    IOTask task(&series->m_writable, param);
    IOHandler()->enqueue(task);

    // We cannot call Series::flush now, since the IO handler is still filled
    // from calling flush(Group|File)based, but has not been emptied yet
    // Do that manually
    IOHandler()->flush(flushParams);

    return *param.status;
}

void Series::flushStep(bool doFlush)
{
    auto &series = get();
    if (!series.m_currentlyActiveIterations.empty() &&
        access::write(IOHandler()->m_frontendAccess))
    {
        /*
         * Warning: changing attribute extents over time (probably) unsupported
         * by this so far.
         * Not (yet) needed as there is no way to pack several iterations within
         * one IO step.
         */
        Parameter<Operation::WRITE_ATT> wAttr;
        wAttr.changesOverSteps =
            Parameter<Operation::WRITE_ATT>::ChangesOverSteps::Yes;
        wAttr.name = "snapshot";
        wAttr.setResource(
            std::vector<unsigned long long>{
                series.m_currentlyActiveIterations.begin(),
                series.m_currentlyActiveIterations.end()});
        series.m_currentlyActiveIterations.clear();
        wAttr.dtype = Datatype::VEC_ULONGLONG;
        IOHandler()->enqueue(IOTask(&series.iterations, wAttr));
        if (doFlush)
        {
            IOHandler()->flush(internal::defaultFlushParams);
        }
    }
    series.m_wroteAtLeastOneIOStep = true;
}

Series &Series::setIterationEncoding_internal(
    IterationEncoding ie, internal::default_or_explicit doe)
{
    auto &series = get();
    switch (doe)
    {
    case internal::default_or_explicit::default_:
    case internal::default_or_explicit::explicit_:
        // mark this option as set explicitly by the user
        series.m_iterationEncodingSetExplicitly = doe;
        break;
    }
    if (series.m_deferred_initialization)
    {
        runDeferredInitialization();
    }
    if (written())
        throw std::runtime_error(
            "A files iterationEncoding can not (yet) be changed after it has "
            "been written.");

    series.m_iterationEncoding = ie;
    switch (ie)
    {
    case IterationEncoding::fileBased:
        setIterationFormat(series.m_name);
        setAttribute("iterationEncoding", std::string("fileBased"));
        // This checks that the name contains the expansion pattern
        // (e.g. %T) and parses it
        if (series.m_filenamePadding < 0)
        {
            if (!reparseExpansionPattern(series.m_name))
            {
                throw error::WrongAPIUsage(
                    "For fileBased formats the iteration expansion pattern "
                    "%T must "
                    "be included in the file name");
            }
        }
        break;
    case IterationEncoding::groupBased:
        setIterationFormat(BASEPATH);
        setAttribute("iterationEncoding", std::string("groupBased"));
        break;
    case IterationEncoding::variableBased:
        setIterationFormat(auxiliary::replace_first(basePath(), "/%T/", ""));
        setAttribute("iterationEncoding", std::string("variableBased"));
        break;
    }
    IOHandler()->setIterationEncoding(ie);
    return *this;
}

auto Series::openIterationIfDirty(IterationIndex_t index, Iteration &iteration)
    -> IterationOpened
{
    auto &data = iteration.get();
    /*
     * Check side conditions on accessing iterations, and if they are fulfilled,
     * forward function params to openIteration().
     */
    if (data.m_closed == internal::CloseStatus::ParseAccessDeferred)
    {
        return IterationOpened::RemainsClosed;
    }
    bool dirtyRecursive = iteration.dirtyRecursive();

    if (data.m_closed == internal::CloseStatus::Closed)
    {
        // file corresponding with the iteration has previously been
        // closed and fully flushed
        // verify that there have been no further accesses
        if (!iteration.written())
        {
            throw std::runtime_error(
                "[Series] Closed iteration has not been written. This "
                "is an internal error.");
        }
        else if (!dirtyRecursive)
        {
            return IterationOpened::RemainsClosed;
        }
        else if (!data.allow_reopening_implicitly)
        {
            throw error::WrongAPIUsage(
                "[Series] Closed iteration (idx=" + std::to_string(index) +
                ") must be open()ed explicitly before interacting with it "
                "again.");
        }
        else
        {
            data.allow_reopening_implicitly = false; // only allow this once
        }
    }

    /*
     * When using writeIterations(), the currently active Iteration should
     * always be flushed (unless, as checked above, it is already closed).
     * This block checks if an Iteration is currently collectively opened.
     */
    [&]() {
        auto &series = get();
        if (!series.m_sharedStatefulIterator)
        {
            return;
        }
        auto &shared_iterator = *series.m_sharedStatefulIterator;
        if (!shared_iterator.m_data || !shared_iterator.m_data->has_value())
        {
            return;
        }
        auto const current_iteration =
            (*shared_iterator.m_data)->currentIteration();
        dirtyRecursive |=
            current_iteration.has_value() && *current_iteration == index;
    }();

    switch (iterationEncoding())
    {
        using IE = IterationEncoding;
    case IE::fileBased:
        /*
         * Opening a file is expensive, so let's do it only if necessary.
         * Necessary if:
         * 1. The iteration itself has been changed somewhere.
         * 2. Or the Series has been changed globally in a manner that
         *    requires adapting all iterations.
         */
        if (dirtyRecursive || this->dirty())
        {
            // openIteration() will update the close status
            openIteration(index, iteration);
            return IterationOpened::HasBeenOpened;
        }
        break;
    case IE::groupBased:
    case IE::variableBased:
        // open unconditionally
        // this makes groupBased encoding safer for parallel usage
        // (variable-based encoding runs in lockstep anyway)
        // openIteration() will update the close status
        openIteration(index, iteration);
        return IterationOpened::HasBeenOpened;
    }
    return IterationOpened::RemainsClosed;
}

void Series::openIteration(IterationIndex_t index, Iteration &iteration)
{
    auto oldStatus = iteration.get().m_closed;
    using CL = internal::CloseStatus;
    /*
     * Closed and ClosedInFrontend need to be treated different here.
     * Closed means that the Iteration is actually closed, but we need it again,
     * so it should be opened again.
     * ClosedInFrontend means that the Iteration is about to be closed, but
     * still open. Nothing needs to be done, the enqueued operations can be
     * performed, and the Iteration will be closed afterwards.
     */
    switch (oldStatus)
    {
    case CL::Closed:
        if (access::writeOnly(IOHandler()->m_frontendAccess))
        {
            std::cerr << &R"(
[Series::openIteration]
  Warning: Reopening closed Iterations in write modes is currently experimental.
  Note that an ADIOS2 step/file cannot be modified once closed, just appended
  to with a new step. Support for this is not yet feature-complete (pre-alpha).
)"[1];
        }
        [[fallthrough]];
    case CL::Open:
        iteration.get().m_closed = CL::Open;
        break;
    case CL::ParseAccessDeferred:
    case CL::ClosedInFrontend:
        // just keep it like it is
        break;
    }

    /*
     * There's only something to do in filebased encoding in READ_ONLY and
     * READ_WRITE modes.
     * Use two nested switches anyway to ensure compiler warnings upon adding
     * values to the enums.
     */
    switch (iterationEncoding())
    {
        using IE = IterationEncoding;
    case IE::fileBased: {
        /*
         * The iteration is marked written() as soon as its file has been
         * either created or opened.
         * If the iteration has not been created yet, it cannot be opened.
         * In that case, it is not written() and its old close status was
         * not ParseAccessDeferred.
         * Similarly, in Create mode, the iteration must first be created
         * before it is possible to open it.
         */
        if (!iteration.written() &&
            (access::create(IOHandler()->m_frontendAccess) ||
             oldStatus != internal::CloseStatus::ParseAccessDeferred))
        {
            // nothing to do, file will be opened by writing routines
            break;
        }
        auto &series = get();
        // open the iteration's file again
        Parameter<Operation::OPEN_FILE> fOpen;
        fOpen.name = iterationFilename(index);
        using R = Parameter<Operation::OPEN_FILE>::Reopen;
        if (oldStatus != CL::Closed)
        {
            fOpen.reopen = R::NoReopen;
        }
        else if (
            // The filename only gets emplaced in there if we found it on the
            // file system, otherwise it's generated by iterationFilename().
            // This helps us distinguish which iterations were created by us and
            // which ones existed already. This is important for ADIOS2 which
            // can open an iteration for Appending XOR for reading.
            series.m_iterationFilenames.find(index) ==
            series.m_iterationFilenames.end())
        {
            fOpen.reopen = R::WasCreatedByUs;
        }
        else
        {
            fOpen.reopen = R::WasFoundOnDisk;
        }
        IOHandler()->enqueue(IOTask(this, fOpen));

        /* open base path */
        Parameter<Operation::OPEN_PATH> pOpen;
        pOpen.path = auxiliary::replace_first(basePath(), "%T/", "");
        IOHandler()->enqueue(IOTask(&series.iterations, pOpen));
        /* open iteration path */
        pOpen.path = iterationEncoding() == IterationEncoding::variableBased
            ? ""
            : std::to_string(index);
        IOHandler()->enqueue(IOTask(&iteration, pOpen));
        break;
    }
    case IE::groupBased:
    case IE::variableBased:
        // nothing to do, no opening necessary in those modes
        break;
    }
}

namespace
{
    /**
     * Look up if the specified key is contained in the JSON dataset.
     * If yes, read it into the specified location.
     */
    template <typename From, typename Dest = From>
    void getJsonOption(
        json::TracingJSON &config,
        std::string const &key,
        Dest &dest,
        std::optional<std::string> envVar = std::nullopt)
    {
        if (envVar.has_value())
        {
            dest = auxiliary::getEnvNum(*envVar, dest);
        }
        if (config.json().contains(key))
        {
            dest = config[key].json().get<From>();
        }
    }

    /**
     * Like getJsonOption(), but for string types.
     * Numbers and booleans are converted to their string representation.
     * The string is converted to lower case.
     */
    template <typename Dest = std::string>
    bool getJsonOptionLowerCase(
        json::TracingJSON &config, std::string const &key, Dest &dest)
    {
        if (config.json().contains(key))
        {
            auto maybeString =
                json::asLowerCaseStringDynamic(config[key].json());
            if (maybeString.has_value())
            {
                dest = std::move(maybeString.value());
            }
            else
            {
                throw error::BackendConfigSchema(
                    {key}, "Must be convertible to string type.");
            }
            return true;
        }
        else
        {
            return false;
        }
    }
} // namespace

template <typename TracingJSON>
void Series::parseJsonOptions(TracingJSON &options, ParsedInput &input)
{
    auto &series = get();
    getJsonOption<bool>(
        options,
        "defer_iteration_parsing",
        series.m_parseLazily,
        "OPENPMD_DEFER_ITERATION_PARSING");
    getJsonOption<uint64_t>(
        options,
        "hint_lazy_parsing_timeout",
        series.m_hintLazyParsingAfterTimeout,
        "OPENPMD_HINT_LAZY_PARSING_TIMEOUT");
    getJsonOption<bool>(
        options,
        "verify_homogeneous_extents",
        input.verify_homogeneous_extents,
        "OPENPMD_VERIFY_HOMOGENEOUS_EXTENTS");
    internal::SeriesData::SourceSpecifiedViaJSON rankTableSource;
    if (getJsonOptionLowerCase(options, "rank_table", rankTableSource.value))
    {
        series.m_rankTable.m_rankTableSource = std::move(rankTableSource);
    }
    // backend key
    {
        std::map<std::string, Format> const backendDescriptors{
            {"hdf5", Format::HDF5},
            {"adios2", Format::ADIOS2_BP},
            {"json", Format::JSON},
            {"toml", Format::TOML}};
        std::string backend;
        getJsonOptionLowerCase(options, "backend", backend);
        if (!backend.empty())
        {
            auto it = backendDescriptors.find(backend);
            if (it != backendDescriptors.end())
            {
                if (backend == "adios2" &&
                    (input.format == Format::ADIOS2_BP4 ||
                     input.format == Format::ADIOS2_BP5 ||
                     input.format == Format::ADIOS2_SST ||
                     input.format == Format::ADIOS2_SSC ||
                     input.format == Format::ADIOS2_BP))
                {
                    // backend = "adios2" should work as a catch-all for all
                    // different engines, using BP is just the default
                    // If the file ending was more explicit, keep it.
                    // -> Nothing to do
                }
                else if (
                    input.format != Format::DUMMY &&
                    input.format != Format::GENERIC &&
                    suffix(input.format) != suffix(it->second))
                {
                    std::cerr << "[Warning] Supplied filename extension '"
                              << suffix(input.format)
                              << "' contradicts the backend specified via the "
                                 "'backend' key. Will go on with backend "
                              << it->first << "." << std::endl;
                    input.format = it->second;
                }
                else
                {
                    input.format = it->second;
                }
            }
            else
            {
                throw error::BackendConfigSchema(
                    {"backend"}, "Unknown backend specified: " + backend);
            }
        }
    }
    // iteration_encoding key
    {
        std::map<std::string, IterationEncoding> const ieDescriptors{
            {"file_based", IterationEncoding::fileBased},
            {"group_based", IterationEncoding::groupBased},
            {"variable_based", IterationEncoding::variableBased}};
        std::string iterationEncoding;
        getJsonOptionLowerCase(
            options, "iteration_encoding", iterationEncoding);
        if (!iterationEncoding.empty())
        {
            series.m_iterationEncodingSetExplicitly =
                internal::default_or_explicit::explicit_;
            auto it = ieDescriptors.find(iterationEncoding);
            if (it != ieDescriptors.end())
            {
                input.iterationEncoding = it->second;
            }
            else
            {
                throw error::BackendConfigSchema(
                    {"iteration_encoding"},
                    "Unknown iteration encoding specified: " +
                        iterationEncoding);
            }
        }
    }
}

namespace internal
{
    SeriesData::~SeriesData()
    {
        // we must not throw in a destructor
        try
        {
            close();
        }
        catch (std::exception const &ex)
        {
            std::cerr << "[~Series] An error occurred: " << ex.what()
                      << std::endl;
        }
        catch (...)
        {
            std::cerr << "[~Series] An error occurred." << std::endl;
        }
    }

    void SeriesData::close()
    {
        // WriteIterations gets the first shot at flushing
        if (this->m_sharedStatefulIterator)
        {
            this->m_sharedStatefulIterator->close();
            this->m_sharedStatefulIterator.reset();
        }
        Series impl;
        impl.setData({this, [](auto const *) {}});
        auto IOHandler = impl.IOHandler();
        if (IOHandler &&
            /*
             * Scenario: A user calls `Series::flush()` but does not check for
             * thrown exceptions. The exception will propagate further up,
             * usually thereby popping the stack frame that holds the `Series`
             * object. `Series::~Series()` will run. This check avoids that the
             * `Series` is needlessly flushed a second time. Otherwise, error
             * messages can get very confusing.
             */

            IOHandler->m_lastFlushSuccessful &&
            /*
             * If a read-only Series is opened without any backend access, then
             * don't go there now. Just peacefully close.
             */
            !(access::readOnly(IOHandler->m_frontendAccess) &&
              !(*this)->m_writable.written))
        {
            impl.flush();
            /*
             * In file-based iteration encoding, this must be triggered by
             * Iteration::endStep() since the "snapshot" attribute is different
             * for each file.
             * Also, at this point the files might have already been closed.
             */
            if (impl.iterationEncoding() != IterationEncoding::fileBased)
            {
                impl.flushStep(/* doFlush = */ true);
            }
        }
        // Not strictly necessary, but clear the map of iterations
        // This releases the openPMD hierarchy
        iterations.container().clear();
        // Release the IO Handler
        if (IOHandler)
        {
            *operator*().m_writable.IOHandler = std::nullopt;
        }
    }
} // namespace internal

Series::Series() : Attributable(NoInit()), iterations{}
{}

#if openPMD_HAVE_MPI
Series::Series(
    std::string const &filepath,
    Access at,
    MPI_Comm comm,
    std::string const &options)
    : Attributable(NoInit())
{
    auto data = std::make_shared<internal::SeriesData>();
    data->m_communicator = comm;
    setData(std::move(data));
    init(filepath, at, options, comm);
}
#endif

Series::Series(
    std::string const &filepath, Access at, std::string const &options)
    : Attributable(NoInit())
{
    setData(std::make_shared<internal::SeriesData>());
    init(filepath, at, options);
}

Series::operator bool() const
{
    return m_attri.operator bool();
}

ReadIterations Series::readIterations()
{
    // Use private constructor instead of copy constructor to avoid
    // object slicing
    auto series = m_attri->asInternalCopyOf<Series>();
    return ReadIterations{
        std::move(series),
        IOHandler()->m_frontendAccess,
        get().m_parsePreference};
}

namespace
{
    auto make_writing_stateful_iterator(
        Series const &copied_series, internal::SeriesData &series)
        -> StatefulIterator *
    {
        if (!series.m_sharedStatefulIterator)
        {
            series.m_sharedStatefulIterator =
                std::make_unique<StatefulIterator>(
                    StatefulIterator::tag_write, copied_series);
        }
        return series.m_sharedStatefulIterator.get();
    }
    auto make_reading_stateful_iterator(
        Series copied_series, internal::SeriesData &series)
        -> std::variant<std::function<StatefulIterator *()>, StatefulIterator *>
    {
        if (series.m_sharedStatefulIterator)
        {
            return series.m_sharedStatefulIterator.get();
        }
        return [s = std::move(copied_series), &series]() mutable {
            if (!series.m_sharedStatefulIterator)
            {
                auto parse_preference = series.m_parsePreference;
                series.m_sharedStatefulIterator =
                    std::make_unique<StatefulIterator>(
                        StatefulIterator::tag_read,
                        std::move(s),
                        parse_preference);
            }
            return series.m_sharedStatefulIterator.get();
        };
    }
} // namespace

Snapshots Series::snapshots()
{
    auto &series = get();
    if (series.m_deferred_initialization.has_value())
    {
        runDeferredInitialization();
    }
    auto access = IOHandler()->m_frontendAccess;
    SnapshotWorkflow usedSnapshotWorkflow = access::random_access(access)
        ? SnapshotWorkflow::RandomAccess
        : SnapshotWorkflow::Synchronous;
    if (access == Access::READ_RANDOM_ACCESS)
    {
        // Some error checks
        if (series.m_parsePreference.has_value())
        {
            switch (series.m_parsePreference.value())
            {
            case internal::ParsePreference::UpFront:
                break;
            case internal::ParsePreference::PerStep:
                throw error::ReadError(
                    error::AffectedObject::File,
                    error::Reason::UnexpectedContent,
                    std::nullopt,
                    "[Series::snapshots()] Series requires collective "
                    "processing with READ_LINEAR access mode.");
            }
        }
        else if (iterationEncoding() != IterationEncoding::fileBased)
        {
            throw error::Internal(
                "READ_ONLY mode and non-fileBased iteration encoding, but "
                "the backend did not set a parse preference.");
        }
    }

    switch (usedSnapshotWorkflow)
    {
    case SnapshotWorkflow::RandomAccess: {
        return makeRandomAccessSnapshots();
    }
    case SnapshotWorkflow::Synchronous: {
        return makeSynchronousSnapshots();
    }
    }
    throw std::runtime_error("unreachable!");
}

Snapshots Series::makeRandomAccessSnapshots()
{
    auto &series = get();
    return Snapshots(
        std::shared_ptr<RandomAccessIteratorContainer>{
            new RandomAccessIteratorContainer(series.iterations)},
        series.iterations);
}
Snapshots Series::makeSynchronousSnapshots()
{
    auto &series = get();
    /*
     * ADIOS2 should use variable-based encoding as a default when
     * applicable, since group-based encoding has severe limitations
     * in ADIOS2. The below logic checks if variable-based encoding
     * should be used.
     */

    if (
        // 1. No encoding has been explicitly selected by the user.
        //    Flag set by Series::setIterationEncoding().
        series.m_iterationEncodingSetExplicitly ==
            internal::default_or_explicit::default_ &&
        // 2. Iteration encoding was recognized as groupBased by
        // init()
        //    procedures (and not file-based).
        series.m_iterationEncoding == IterationEncoding::groupBased &&
        // 3. The chosen access type is write-only, otherwise the
        // encoding is
        //    determined by the previous file content.
        access::writeOnly(IOHandler()->m_frontendAccess) &&
        // 4. The backend is ADIOS2 in a recent enough version to
        // support
        //    modifiable attributes (v2.9).
        IOHandler()->fullSupportForVariableBasedEncoding() &&
        // 5. The Series must not yet be written, otherwise we're
        // too late
        //    for this
        !this->written())
    {
        setIterationEncoding_internal(
            IterationEncoding::variableBased,
            internal::default_or_explicit::default_);
    }

    std::variant<std::function<StatefulIterator *()>, StatefulIterator *> begin;

    if (access::write(IOHandler()->m_frontendAccess))
    {
        begin = make_writing_stateful_iterator(*this, series);
    }
    else
    {
        begin = make_reading_stateful_iterator(*this, series);
    }
    return Snapshots(
        std::shared_ptr<StatefulSnapshotsContainer>(
            new StatefulSnapshotsContainer(std::move(begin))),
        series.iterations);
}

void Series::parseBase()
{
    readIterations();
}

WriteIterations Series::writeIterations()
{
    auto const access = IOHandler()->m_frontendAccess;
    if (access != Access::CREATE_RANDOM_ACCESS &&
        access != Access::APPEND_RANDOM_ACCESS &&
        access != Access::CREATE_LINEAR &&
        access != Access::CREATE_RANDOM_ACCESS)
    {
        throw error::WrongAPIUsage(
            "[Series::writeIterations()] May only be applied for access modes "
            "CREATE or APPEND. Use Series::snapshots() for random-access-type "
            "or for read-type workflows.");
    }
    return makeSynchronousSnapshots();
}

void Series::close()
{
    get().close();
    m_attri.reset();
}

auto Series::currentSnapshot() -> std::optional<std::vector<IterationIndex_t>>
{
    using vec_t = std::vector<IterationIndex_t>;
    auto &series = get();
    /*
     * In variable-based iteration encoding, iterations have no distinct
     * group within `series.iterations`, meaning that the `snapshot`
     * attribute is not found at `/data/0/snapshot`, but at
     * `/data/snapshot`. This makes it possible to retrieve it from
     * `series.iterations`.
     */
    if (!series.iterations.containsAttribute("snapshot"))
    {
        return std::nullopt;
    }
    else if (randomAccessSteps())
    {
        auto preparsed = preparseSnapshots();
        if (!preparsed.has_value())
        {
            return std::nullopt;
        }
        std::set<IterationIndex_t> res;
        for (size_t step = 0; step < preparsed->size(); ++step)
        {
            for (IterationIndex_t iteration : (*preparsed)[step])
            {
                res.emplace(iteration);
                // If an Iteration is found in multiple steps, this logic will
                // prefer the Iteration from the later step.
                series.m_snapshotToStep[iteration] = step;
            }
        }
        std::cout.flush();
        return vec_t{res.begin(), res.end()};
    }
    else
    {
        auto const &attribute = series.iterations.getAttribute("snapshot");
        auto res = attribute.getOptional<vec_t>();
        if (res.has_value())
        {
            return res;
        }
        else
        {
            std::stringstream s;
            s << "Unexpected datatype for '/data/snapshot': " << attribute.dtype
              << " (expected a vector of integer, found " +
                    datatypeToString(attribute.dtype) + ")"
              << std::endl;
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                s.str());
        }
    }
}

AbstractIOHandler *Series::runDeferredInitialization()
{
    auto &series = get();
    if (series.m_deferred_initialization.has_value())
    {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto functor = std::move(*m_series->m_deferred_initialization);
        m_series->m_deferred_initialization = std::nullopt;
        return functor(*this);
    }
    else
    {
        return nullptr;
    }
}

AbstractIOHandler *Series::IOHandler()
{
    auto res = Attributable::IOHandler();
    if (res && //  res->backendName() == "Dummy" &&
        m_series->m_deferred_initialization.has_value())
    {
        res = runDeferredInitialization();
    }
    return res;
}
AbstractIOHandler const *Series::IOHandler() const
{
    auto res = Attributable::IOHandler();
    return res;
}

auto Series::preparseSnapshots()
    -> std::optional<std::vector<std::vector<IterationIndex_t>>>
{
    using vec_t = std::vector<IterationIndex_t>;
    auto &series = get();
    if (!series.m_snapshotToStep.empty())
    {
        throw error::Internal(
            "Control flow error: This is an expensive operation and should be "
            "called only once upon initialization.");
    }
    auto io_handler = IOHandler();
    if (!series.iterations.containsAttribute("snapshot"))
    {
        return std::nullopt;
    }
    Parameter<Operation::READ_ATT_ALLSTEPS> readAttr;
    readAttr.name = "snapshot";
    io_handler->enqueue(IOTask(&series.iterations, readAttr));
    io_handler->flush(internal::defaultFlushParams);
    auto wrong_datatype = [&]() {
        std::stringstream s;
        s << "Unexpected datatype for '/data/snapshot': " << *readAttr.dtype
          << " (expected a vector of integer).";
        return error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::UnexpectedContent,
            {},
            s.str());
    };
    return std::visit(
        [&](auto &vec) -> std::vector<vec_t> {
            using containedType =
                typename std::decay_t<decltype(vec)>::value_type;
            if constexpr (std::is_same_v<containedType, bool>)
            {
                // need to keep the vector<bool> overload away from the
                // below implementation
                throw wrong_datatype();
            }
            else
            {
                std::vector<vec_t> res;
                res.reserve(vec.size());
                for (auto const &val : vec)
                {
                    auto converted =
                        detail::doConvertOptional<containedType, vec_t>(&val);
                    if (!converted.has_value())
                    {
                        throw wrong_datatype();
                    }
                    res.emplace_back(*converted);
                }
                return res;
            }
        },
        readAttr.resource<vector_of_attributes_type>());
}

bool Series::randomAccessSteps() const
{
    return get().m_parsePreference.value_or(
               internal::ParsePreference::UpFront) ==
        internal::ParsePreference::UpFront &&
        iterationEncoding() == IterationEncoding::variableBased &&
        access::random_access(IOHandler()->m_backendAccess);
}

namespace
{
    CleanedFilename cleanFilename(
        std::string const &filename, std::string const &filenameExtension)
    {
        std::string postfix =
            auxiliary::replace_last(filename, filenameExtension, "");
        if (postfix == filename)
        {
            return {postfix, ""};
        }
        else
        {
            return {postfix, filenameExtension};
        }
    }

    std::function<Match(std::string const &)> buildMatcher(
        std::string const &regexPattern,
        int padding,
        std::optional<size_t> index_of_extension)
    {
        return [index_of_extension,
                pattern = std::regex(regexPattern),
                padding](std::string const &filename) -> Match {
            std::smatch regexMatches;
            bool match = std::regex_match(filename, regexMatches, pattern);
            int processedPadding =
                padding != 0 ? padding : (match ? regexMatches[2].length() : 0);
            return {
                match,
                processedPadding,
                padding < 0 ? padding
                    : match ? std::stoull(regexMatches[2])
                            : 0,
                index_of_extension.has_value()
                    ? std::make_optional<std::string>(
                          regexMatches[*index_of_extension])
                    : std::nullopt};
        };
    }

    namespace
    {
        auto sanitize_regex(std::string const &input) -> std::string
        {
            // need to escape special characters reserved for regexes, see
            // https://stackoverflow.com/questions/40195412/c11-regex-search-for-exact-string-escape
            // https://regex101.com/r/GDPK7E/3
            std::regex specialChars{R"([-[\]{}()*+?.,\^$|#\s\\])"};
            // `$&` is the matched substring, see
            // https://en.cppreference.com/w/cpp/regex/regex_replace
            return std::regex_replace(input, specialChars, R"(\$&)");
        }
    } // namespace

    std::function<Match(std::string const &)> matcher(
        std::string const &prefix,
        int padding,
        std::string const &postfix,
        std::optional<std::string> const &filenameSuffix)
    {
        std::string nameReg = "^(" + sanitize_regex(prefix) + ")";
        size_t index_of_extension = 0;
        if (padding < 0)
        {
            index_of_extension = 3;
        }
        else if (padding > 0)
        {
            // The part after the question mark:
            // The number must be at least `padding` digits long
            // The part before the question mark:
            // It may be longer than that only if the first digit is not zero
            // The outer pair of parentheses is for later extraction of the
            // iteration number via std::stoull(regexMatches[1])
            nameReg += "(([1-9][[:digit:]]*)?([[:digit:]]";
            nameReg += "{" + std::to_string(padding) + "}))";
            index_of_extension = 6;
        }
        else
        {
            // No padding specified, any number of digits is ok.
            nameReg += "([[:digit:]]";
            nameReg += "+)";
            index_of_extension = 4;
        }
        nameReg += "(" + sanitize_regex(postfix) + ")" +
            filenameSuffix.value_or("(\\.[[:alnum:]]+)") + "$";
        return buildMatcher(
            nameReg,
            padding,
            !filenameSuffix.has_value()
                ? std::make_optional<size_t>(index_of_extension)
                : std::nullopt);
    }
} // namespace

namespace debug
{
    void printDirty(Series const &series)
    {
        auto print = [](Attributable const &attr) {
            size_t indent = 0;
            {
                auto current = attr.parent();
                while (current)
                {
                    ++indent;
                    current = current->parent;
                }
            }
            auto make_indent = [&]() {
                for (size_t i = 0; i < indent; ++i)
                {
                    std::cout << "\t";
                }
            };
            make_indent();
            auto const &w = attr.writable();
            std::cout << w.ownKeyWithinParent << '\n';
            make_indent();
            std::cout << "Self: " << w.dirtySelf
                      << "\tRec: " << w.dirtyRecursive << '\n';
            std::cout << std::endl;
        };
        print(series);
        print(series.iterations);
        for (auto const &[it_name, it] : series.iterations)
        {
            (void)it_name;
            print(it);
            print(it.meshes);
            for (auto const &[mesh_name, mesh] : it.meshes)
            {
                (void)mesh_name;
                print(mesh);
                if (!mesh.scalar())
                {
                    for (auto const &[comp_name, comp] : mesh)
                    {
                        (void)comp_name;
                        print(comp);
                    }
                }
            }
            print(it.particles);
            for (auto const &[species_name, species] : it.particles)
            {
                (void)species_name;
                print(species);
                print(species.particlePatches);
                for (auto const &[patch_name, patch] : species.particlePatches)
                {
                    (void)patch_name;
                    print(patch);
                    if (!patch.scalar())
                    {
                        for (auto const &[component_name, component] : patch)
                        {
                            (void)component_name;
                            print(component);
                        }
                    }
                }
                for (auto const &[record_name, record] : species)
                {
                    (void)record_name;
                    print(record);
                    if (!record.scalar())
                    {
                        for (auto const &[comp_name, comp] : record)
                        {
                            (void)comp_name;
                            print(comp);
                        }
                    }
                }
            }
        }
    }
} // namespace debug
} // namespace openPMD
