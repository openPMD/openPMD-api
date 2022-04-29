/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/ReadIterations.hpp"
#include "openPMD/auxiliary/Date.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/version.hpp"

#include <cctype>
#include <exception>
#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <tuple>
#include <utility>

namespace openPMD
{
namespace
{
    /** Remove the filename extension of a given storage format.
     *
     * @param   filename    String containing the filename, possibly with
     * filename extension.
     * @param   f           File format to remove filename extension for.
     * @return  String containing the filename without filename extension.
     */
    std::string cleanFilename(std::string const &filename, Format f);

    /** Compound return type for regex matching of filenames */
    struct Match
    {
        bool isContained{}; //! pattern match successful
        int padding{}; //! number of zeros used for padding of iteration
        uint64_t iteration{}; //! iteration found in regex pattern (default: 0)

        // support for std::tie
        operator std::tuple<bool &, int &, uint64_t &>()
        {
            return std::tuple<bool &, int &, uint64_t &>{
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
     * @param   f           File format to check backend applicability for.
     * @return  Functor returning tuple of bool and int.
     *          bool is True if file could be of type f and matches the
     * iterationEncoding. False otherwise. int is the amount of padding present
     * in the iteration number %T. Is 0 if bool is False.
     */
    std::function<Match(std::string const &)> matcher(
        std::string const &prefix,
        int padding,
        std::string const &postfix,
        Format f);
} // namespace

struct Series::ParsedInput
{
    std::string path;
    std::string name;
    Format format;
    IterationEncoding iterationEncoding;
    std::string filenamePrefix;
    std::string filenamePostfix;
    int filenamePadding = -1;
}; // ParsedInput

std::string Series::openPMD() const
{
    return getAttribute("openPMD").get<std::string>();
}

Series &Series::setOpenPMD(std::string const &o)
{
    setAttribute("openPMD", o);
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
    std::string version = openPMD();
    if (version == "1.0.0" || version == "1.0.1" || version == "1.1.0")
        throw std::runtime_error(
            "Custom basePath not allowed in openPMD <=1.1.0");

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
            [](Container<Iteration, uint64_t>::value_type const &i) {
                return i.second.meshes.written();
            }))
        throw std::runtime_error(
            "A files meshesPath can not (yet) be changed after it has been "
            "written.");

    if (auxiliary::ends_with(mp, '/'))
        setAttribute("meshesPath", mp);
    else
        setAttribute("meshesPath", mp + "/");
    dirty() = true;
    return *this;
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
            [](Container<Iteration, uint64_t>::value_type const &i) {
                return i.second.particles.written();
            }))
        throw std::runtime_error(
            "A files particlesPath can not (yet) be changed after it has been "
            "written.");

    if (auxiliary::ends_with(pp, '/'))
        setAttribute("particlesPath", pp);
    else
        setAttribute("particlesPath", pp + "/");
    dirty() = true;
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
    auto &series = get();
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
        if (basePath() != i && (openPMD() == "1.0.1" || openPMD() == "1.0.0"))
            throw std::invalid_argument(
                "iterationFormat must not differ from basePath " + basePath() +
                " for group- or variableBased data");

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
    dirty() = true;
    return *this;
}

std::string Series::backend() const
{
    return IOHandler()->backendName();
}

void Series::flush()
{
    auto &series = get();
    flush_impl(
        series.iterations.begin(),
        series.iterations.end(),
        {FlushLevel::UserFlush});
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
        cleanFilename(input->filenamePostfix, input->format);

    input->name = cleanFilename(input->name, input->format);

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

void Series::init(
    std::shared_ptr<AbstractIOHandler> ioHandler,
    std::unique_ptr<Series::ParsedInput> input)
{
    auto &series = get();
    writable().IOHandler = ioHandler;
    series.iterations.linkHierarchy(writable());
    series.iterations.writable().ownKeyWithinParent = {"iterations"};

    series.m_name = input->name;

    series.m_format = input->format;

    series.m_filenamePrefix = input->filenamePrefix;
    series.m_filenamePostfix = input->filenamePostfix;
    series.m_filenamePadding = input->filenamePadding;

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

    if (IOHandler()->m_frontendAccess == Access::READ_ONLY ||
        IOHandler()->m_frontendAccess == Access::READ_WRITE)
    {
        /* Allow creation of values in Containers and setting of Attributes
         * Would throw for Access::READ_ONLY */
        auto oldType = IOHandler()->m_frontendAccess;
        auto newType = const_cast<Access *>(&IOHandler()->m_frontendAccess);
        *newType = Access::READ_WRITE;

        if (input->iterationEncoding == IterationEncoding::fileBased)
            readFileBased();
        else
            readGorVBased();

        if (series.iterations.empty())
        {
            /* Access::READ_WRITE can be used to create a new Series
             * allow setting attributes in that case */
            written() = false;

            initDefaults(input->iterationEncoding);
            setIterationEncoding(input->iterationEncoding);

            written() = true;
        }

        *newType = oldType;
    }
    else
    {
        initDefaults(input->iterationEncoding);
        setIterationEncoding(input->iterationEncoding);
    }
    series.m_lastFlushSuccessful = true;
}

void Series::initDefaults(IterationEncoding ie)
{
    if (!containsAttribute("openPMD"))
        setOpenPMD(getStandard());
    if (!containsAttribute("openPMDextension"))
        setOpenPMDextension(0);
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
    if (!containsAttribute("date"))
        setDate(auxiliary::getDateString());
    if (!containsAttribute("software"))
        setSoftware("openPMD-api", getVersion());
}

std::future<void> Series::flush_impl(
    iterations_iterator begin,
    iterations_iterator end,
    internal::FlushParams flushParams,
    bool flushIOHandler)
{
    auto &series = get();
    series.m_lastFlushSuccessful = true;
    try
    {
        switch (iterationEncoding())
        {
            using IE = IterationEncoding;
        case IE::fileBased:
            flushFileBased(begin, end, flushParams);
            break;
        case IE::groupBased:
        case IE::variableBased:
            flushGorVBased(begin, end, flushParams);
            break;
        }
        if (flushIOHandler)
        {
            return IOHandler()->flush(flushParams);
        }
        else
        {
            return {};
        }
    }
    catch (...)
    {
        series.m_lastFlushSuccessful = false;
        throw;
    }
}

void Series::flushFileBased(
    iterations_iterator begin,
    iterations_iterator end,
    internal::FlushParams flushParams)
{
    auto &series = get();
    if (end == begin)
        throw std::runtime_error(
            "fileBased output can not be written with no iterations.");

    if (IOHandler()->m_frontendAccess == Access::READ_ONLY)
        for (auto it = begin; it != end; ++it)
        {
            // Phase 1
            switch (openIterationIfDirty(it->first, it->second))
            {
                using IO = IterationOpened;
            case IO::HasBeenOpened:
                it->second.flush(flushParams);
                break;
            case IO::RemainsClosed:
                break;
            }

            // Phase 2
            if (it->second.get().m_closed ==
                internal::CloseStatus::ClosedInFrontend)
            {
                Parameter<Operation::CLOSE_FILE> fClose;
                IOHandler()->enqueue(IOTask(&it->second, std::move(fClose)));
                it->second.get().m_closed =
                    internal::CloseStatus::ClosedInBackend;
            }

            // Phase 3
            IOHandler()->flush(flushParams);
        }
    else
    {
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
                 * written
                 */
                written() = false;
                series.iterations.written() = false;

                dirty() |= it->second.dirty();
                std::string filename = iterationFilename(it->first);
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
                it->second.get().m_closed =
                    internal::CloseStatus::ClosedInBackend;
            }

            // Phase 3
            IOHandler()->flush(flushParams);

            /* reset the dirty bit for every iteration (i.e. file)
             * otherwise only the first iteration will have updates attributes
             */
            dirty() = allDirty;
        }
        dirty() = false;
    }
}

void Series::flushGorVBased(
    iterations_iterator begin,
    iterations_iterator end,
    internal::FlushParams flushParams)
{
    auto &series = get();
    if (IOHandler()->m_frontendAccess == Access::READ_ONLY)
        for (auto it = begin; it != end; ++it)
        {
            // Phase 1
            switch (openIterationIfDirty(it->first, it->second))
            {
                using IO = IterationOpened;
            case IO::HasBeenOpened:
                it->second.flush(flushParams);
                break;
            case IO::RemainsClosed:
                break;
            }

            // Phase 2
            if (it->second.get().m_closed ==
                internal::CloseStatus::ClosedInFrontend)
            {
                // the iteration has no dedicated file in group-based mode
                it->second.get().m_closed =
                    internal::CloseStatus::ClosedInBackend;
            }

            // Phase 3
            IOHandler()->flush(flushParams);
        }
    else
    {
        if (!written())
        {
            Parameter<Operation::CREATE_FILE> fCreate;
            fCreate.name = series.m_name;
            fCreate.encoding = iterationEncoding();
            IOHandler()->enqueue(IOTask(this, fCreate));
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
                it->second.get().m_closed =
                    internal::CloseStatus::ClosedInBackend;
            }
        }

        flushAttributes(flushParams);
        IOHandler()->flush(flushParams);
    }
}

void Series::flushMeshesPath()
{
    Parameter<Operation::WRITE_ATT> aWrite;
    aWrite.name = "meshesPath";
    Attribute a = getAttribute("meshesPath");
    aWrite.resource = a.getResource();
    aWrite.dtype = a.dtype;
    IOHandler()->enqueue(IOTask(this, aWrite));
}

void Series::flushParticlesPath()
{
    Parameter<Operation::WRITE_ATT> aWrite;
    aWrite.name = "particlesPath";
    Attribute a = getAttribute("particlesPath");
    aWrite.resource = a.getResource();
    aWrite.dtype = a.dtype;
    IOHandler()->enqueue(IOTask(this, aWrite));
}

void Series::readFileBased()
{
    auto &series = get();
    Parameter<Operation::OPEN_FILE> fOpen;
    Parameter<Operation::READ_ATT> aRead;
    fOpen.encoding = iterationEncoding();

    if (!auxiliary::directory_exists(IOHandler()->directory))
        throw no_such_file_error(
            "Supplied directory is not valid: " + IOHandler()->directory);

    auto isPartOfSeries = matcher(
        series.m_filenamePrefix,
        series.m_filenamePadding,
        series.m_filenamePostfix,
        series.m_format);
    bool isContained;
    int padding;
    uint64_t iterationIndex;
    std::set<int> paddings;
    for (auto const &entry : auxiliary::list_directory(IOHandler()->directory))
    {
        std::tie(isContained, padding, iterationIndex) = isPartOfSeries(entry);
        if (isContained)
        {
            Iteration &i = series.iterations[iterationIndex];
            i.deferParseAccess(
                {std::to_string(iterationIndex), iterationIndex, true, entry});
            // TODO skip if the padding is exact the number of chars in an
            // iteration?
            paddings.insert(padding);
        }
    }

    if (series.iterations.empty())
    {
        /* Frontend access type might change during Series::read() to allow
         * parameter modification. Backend access type stays unchanged for the
         * lifetime of a Series. */
        if (IOHandler()->m_backendAccess == Access::READ_ONLY)
            throw no_such_file_error("No matching iterations found: " + name());
        else
            std::cerr << "No matching iterations found: " << name()
                      << std::endl;
    }

    auto readIterationEagerly = [](Iteration &iteration) {
        iteration.runDeferredParseAccess();
        Parameter<Operation::CLOSE_FILE> fClose;
        iteration.IOHandler()->enqueue(IOTask(&iteration, fClose));
        iteration.IOHandler()->flush(internal::defaultFlushParams);
        iteration.get().m_closed = internal::CloseStatus::ClosedTemporarily;
    };
    if (series.m_parseLazily)
    {
        for (auto &iteration : series.iterations)
        {
            iteration.second.get().m_closed =
                internal::CloseStatus::ParseAccessDeferred;
        }
        // open the last iteration, just to parse Series attributes
        auto getLastIteration = series.iterations.end();
        getLastIteration--;
        auto &lastIteration = getLastIteration->second;
        readIterationEagerly(lastIteration);
    }
    else
    {
        for (auto &iteration : series.iterations)
        {
            readIterationEagerly(iteration.second);
        }
    }

    if (paddings.size() == 1u)
        series.m_filenamePadding = *paddings.begin();

    /* Frontend access type might change during Series::read() to allow
     * parameter modification. Backend access type stays unchanged for the
     * lifetime of a Series. */
    if (paddings.size() > 1u &&
        IOHandler()->m_backendAccess == Access::READ_WRITE)
        throw std::runtime_error(
            "Cannot write to a series with inconsistent iteration padding. "
            "Please specify '%0<N>T' or open as read-only.");
}

void Series::readOneIterationFileBased(std::string const &filePath)
{
    auto &series = get();

    Parameter<Operation::OPEN_FILE> fOpen;
    Parameter<Operation::READ_ATT> aRead;

    fOpen.name = filePath;
    IOHandler()->enqueue(IOTask(this, fOpen));
    IOHandler()->flush(internal::defaultFlushParams);
    series.iterations.parent() = getWritable(this);

    readBase();

    using DT = Datatype;
    aRead.name = "iterationEncoding";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::STRING)
    {
        std::string encoding = Attribute(*aRead.resource).get<std::string>();
        if (encoding == "fileBased")
            series.m_iterationEncoding = IterationEncoding::fileBased;
        else if (encoding == "groupBased")
        {
            series.m_iterationEncoding = IterationEncoding::groupBased;
            std::cerr << "Series constructor called with iteration "
                         "regex '%T' suggests loading a "
                      << "time series with fileBased iteration "
                         "encoding. Loaded file is groupBased.\n";
        }
        else if (encoding == "variableBased")
        {
            /*
             * Unlike if the file were group-based, this one doesn't work
             * at all since the group paths are different.
             */
            throw std::runtime_error(
                "Series constructor called with iteration "
                "regex '%T' suggests loading a "
                "time series with fileBased iteration "
                "encoding. Loaded file is variableBased.");
        }
        else
            throw std::runtime_error("Unknown iterationEncoding: " + encoding);
        setAttribute("iterationEncoding", encoding);
    }
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype "
            "for 'iterationEncoding'");

    aRead.name = "iterationFormat";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::STRING)
    {
        written() = false;
        setIterationFormat(Attribute(*aRead.resource).get<std::string>());
        written() = true;
    }
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'iterationFormat'");

    Parameter<Operation::OPEN_PATH> pOpen;
    std::string version = openPMD();
    if (version == "1.0.0" || version == "1.0.1" || version == "1.1.0")
        pOpen.path = auxiliary::replace_first(basePath(), "/%T/", "");
    else
        throw std::runtime_error("Unknown openPMD version - " + version);
    IOHandler()->enqueue(IOTask(&series.iterations, pOpen));

    readAttributes(ReadMode::IgnoreExisting);
    series.iterations.readAttributes(ReadMode::OverrideExisting);
}

void Series::readGorVBased(bool do_init)
{
    auto &series = get();
    Parameter<Operation::OPEN_FILE> fOpen;
    fOpen.name = series.m_name;
    fOpen.encoding = iterationEncoding();
    IOHandler()->enqueue(IOTask(this, fOpen));
    IOHandler()->flush(internal::defaultFlushParams);

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
                Attribute(*aRead.resource).get<std::string>();
            if (encoding == "groupBased")
                series.m_iterationEncoding = IterationEncoding::groupBased;
            else if (encoding == "variableBased")
                series.m_iterationEncoding = IterationEncoding::variableBased;
            else if (encoding == "fileBased")
            {
                series.m_iterationEncoding = IterationEncoding::fileBased;
                std::cerr << "Series constructor called with explicit "
                             "iteration suggests loading a "
                          << "single file with groupBased iteration encoding. "
                             "Loaded file is fileBased.\n";
                /*
                 * We'll want the openPMD API to continue series.m_name to open
                 * the file instead of piecing the name together via
                 * prefix-padding-postfix things.
                 */
                series.m_overrideFilebasedFilename = series.m_name;
            }
            else
                throw std::runtime_error(
                    "Unknown iterationEncoding: " + encoding);
            setAttribute("iterationEncoding", encoding);
        }
        else
            throw std::runtime_error(
                "Unexpected Attribute datatype for 'iterationEncoding'");

        aRead.name = "iterationFormat";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);
        if (*aRead.dtype == DT::STRING)
        {
            written() = false;
            setIterationFormat(Attribute(*aRead.resource).get<std::string>());
            written() = true;
        }
        else
            throw std::runtime_error(
                "Unexpected Attribute datatype for 'iterationFormat'");
    }

    Parameter<Operation::OPEN_PATH> pOpen;
    std::string version = openPMD();
    if (version == "1.0.0" || version == "1.0.1" || version == "1.1.0")
        pOpen.path = auxiliary::replace_first(basePath(), "/%T/", "");
    else
        throw std::runtime_error("Unknown openPMD version - " + version);
    IOHandler()->enqueue(IOTask(&series.iterations, pOpen));

    readAttributes(ReadMode::IgnoreExisting);
    /*
     * 'snapshot' changes over steps, so reread that.
     */
    series.iterations.readAttributes(ReadMode::OverrideExisting);
    /* obtain all paths inside the basepath (i.e. all iterations) */
    Parameter<Operation::LIST_PATHS> pList;
    IOHandler()->enqueue(IOTask(&series.iterations, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    auto readSingleIteration = [&series, &pOpen, this](
                                   uint64_t index,
                                   std::string path,
                                   bool guardAgainstRereading) {
        if (series.iterations.contains(index))
        {
            // maybe re-read
            auto &i = series.iterations.at(index);
            // i.written(): the iteration has already been parsed
            // reparsing is not needed
            if (guardAgainstRereading && i.written())
            {
                return;
            }
            if (i.get().m_closed != internal::CloseStatus::ParseAccessDeferred)
            {
                pOpen.path = path;
                IOHandler()->enqueue(IOTask(&i, pOpen));
                i.reread(path);
            }
        }
        else
        {
            // parse for the first time, resp. delay the parsing process
            Iteration &i = series.iterations[index];
            i.deferParseAccess({path, index, false, ""});
            if (!series.m_parseLazily)
            {
                i.runDeferredParseAccess();
                i.get().m_closed = internal::CloseStatus::Open;
            }
            else
            {
                i.get().m_closed = internal::CloseStatus::ParseAccessDeferred;
            }
        }
    };

    switch (iterationEncoding())
    {
    case IterationEncoding::groupBased:
    /*
     * Sic! This happens when a file-based Series is opened in group-based mode.
     */
    case IterationEncoding::fileBased:
        for (auto const &it : *pList.paths)
        {
            uint64_t index = std::stoull(it);
            readSingleIteration(index, it, true);
        }
        break;
    case IterationEncoding::variableBased: {
        uint64_t index = 0;
        if (series.iterations.containsAttribute("snapshot"))
        {
            index = series.iterations.getAttribute("snapshot").get<uint64_t>();
        }
        readSingleIteration(index, "", false);
        break;
    }
    }
}

void Series::readBase()
{
    auto &series = get();
    using DT = Datatype;
    Parameter<Operation::READ_ATT> aRead;

    aRead.name = "openPMD";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::STRING)
        setOpenPMD(Attribute(*aRead.resource).get<std::string>());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'openPMD'");

    aRead.name = "openPMDextension";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == determineDatatype<uint32_t>())
        setOpenPMDextension(Attribute(*aRead.resource).get<uint32_t>());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'openPMDextension'");

    aRead.name = "basePath";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::STRING)
        setAttribute("basePath", Attribute(*aRead.resource).get<std::string>());
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'basePath'");

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
        if (*aRead.dtype == DT::STRING)
        {
            /* allow setting the meshes path after completed IO */
            for (auto &it : series.iterations)
                it.second.meshes.written() = false;

            setMeshesPath(Attribute(*aRead.resource).get<std::string>());

            for (auto &it : series.iterations)
                it.second.meshes.written() = true;
        }
        else
            throw std::runtime_error(
                "Unexpected Attribute datatype for 'meshesPath'");
    }

    if (std::count(
            aList.attributes->begin(),
            aList.attributes->end(),
            "particlesPath") == 1)
    {
        aRead.name = "particlesPath";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);
        if (*aRead.dtype == DT::STRING)
        {
            /* allow setting the meshes path after completed IO */
            for (auto &it : series.iterations)
                it.second.particles.written() = false;

            setParticlesPath(Attribute(*aRead.resource).get<std::string>());

            for (auto &it : series.iterations)
                it.second.particles.written() = true;
        }
        else
            throw std::runtime_error(
                "Unexpected Attribute datatype for 'particlesPath'");
    }
}

std::string Series::iterationFilename(uint64_t i)
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
    else if (auto iteration = iterations.find(i); //
             iteration != iterations.end() &&
             iteration->second.get().m_overrideFilebasedFilename.has_value())
    {
        return iteration->second.get().m_overrideFilebasedFilename.value();
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
    for (auto it = series.iterations.begin(); it != series.iterations.end();
         ++it)
    {
        if (&it->second.Attributable::get() == &iteration.Attributable::get())
        {
            return it;
        }
    }
    throw std::runtime_error(
        "[Iteration::close] Iteration not found in Series.");
}

AdvanceStatus Series::advance(
    AdvanceMode mode,
    internal::AttributableData &file,
    iterations_iterator begin,
    Iteration &iteration)
{
    constexpr internal::FlushParams flushParams = {FlushLevel::UserFlush};
    auto &series = get();
    auto end = begin;
    ++end;
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

    // @todo really collective?
    flush_impl(begin, end, flushParams, /* flushIOHandler = */ false);

    if (oldCloseStatus == internal::CloseStatus::ClosedInFrontend)
    {
        // Series::flush() would normally turn a `ClosedInFrontend` into
        // a `ClosedInBackend`. Do that manually.
        itData.m_closed = internal::CloseStatus::ClosedInBackend;
    }
    else if (
        oldCloseStatus == internal::CloseStatus::ClosedInBackend &&
        series.m_iterationEncoding == IterationEncoding::fileBased)
    {
        /*
         * In file-based iteration encoding, we want to avoid accidentally
         * opening an iteration's file by beginning a step on it.
         * So, return now.
         */
        return AdvanceStatus::OK;
    }

    Parameter<Operation::ADVANCE> param;
    if (itData.m_closed == internal::CloseStatus::ClosedTemporarily &&
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
        IOTask task(&file.m_writable, param);
        IOHandler()->enqueue(task);
    }

    if (oldCloseStatus == internal::CloseStatus::ClosedInFrontend &&
        mode == AdvanceMode::ENDSTEP)
    {
        using IE = IterationEncoding;
        switch (series.m_iterationEncoding)
        {
        case IE::fileBased: {
            if (itData.m_closed != internal::CloseStatus::ClosedTemporarily)
            {
                Parameter<Operation::CLOSE_FILE> fClose;
                IOHandler()->enqueue(IOTask(&iteration, std::move(fClose)));
            }
            itData.m_closed = internal::CloseStatus::ClosedInBackend;
            break;
        }
        case IE::groupBased: {
            // We can now put some groups to rest
            Parameter<Operation::CLOSE_PATH> fClose;
            IOHandler()->enqueue(IOTask(&iteration, std::move(fClose)));
            // In group-based iteration layout, files are
            // not closed on a per-iteration basis
            // We will treat it as such nonetheless
            itData.m_closed = internal::CloseStatus::ClosedInBackend;
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

auto Series::openIterationIfDirty(uint64_t index, Iteration iteration)
    -> IterationOpened
{
    /*
     * Check side conditions on accessing iterations, and if they are fulfilled,
     * forward function params to openIteration().
     */
    if (iteration.get().m_closed == internal::CloseStatus::ParseAccessDeferred)
    {
        return IterationOpened::RemainsClosed;
    }
    bool const dirtyRecursive = iteration.dirtyRecursive();
    if (iteration.get().m_closed == internal::CloseStatus::ClosedInBackend)
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
        if (dirtyRecursive)
        {
            throw std::runtime_error(
                "[Series] Detected illegal access to iteration that "
                "has been closed previously.");
        }
        return IterationOpened::RemainsClosed;
    }

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

void Series::openIteration(uint64_t index, Iteration iteration)
{
    auto oldStatus = iteration.get().m_closed;
    switch (oldStatus)
    {
        using CL = internal::CloseStatus;
    case CL::ClosedInBackend:
        throw std::runtime_error(
            "[Series] Detected illegal access to iteration that "
            "has been closed previously.");
    case CL::ParseAccessDeferred:
    case CL::Open:
    case CL::ClosedTemporarily:
        iteration.get().m_closed = CL::Open;
        break;
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
            (IOHandler()->m_frontendAccess == Access::CREATE ||
             oldStatus != internal::CloseStatus::ParseAccessDeferred))
        {
            // nothing to do, file will be opened by writing routines
            break;
        }
        auto &series = get();
        // open the iteration's file again
        Parameter<Operation::OPEN_FILE> fOpen;
        fOpen.encoding = iterationEncoding();
        fOpen.name = iterationFilename(index);
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
    void
    getJsonOption(json::TracingJSON &config, std::string const &key, Dest &dest)
    {
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
    void getJsonOptionLowerCase(
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
        }
    }
} // namespace

template <typename TracingJSON>
void Series::parseJsonOptions(TracingJSON &options, ParsedInput &input)
{
    auto &series = get();
    getJsonOption<bool>(
        options, "defer_iteration_parsing", series.m_parseLazily);
    // backend key
    {
        std::map<std::string, Format> const backendDescriptors{
            {"hdf5", Format::HDF5},
            {"adios1", Format::ADIOS1},
            {"adios2", Format::ADIOS2},
            {"json", Format::JSON}};
        std::string backend;
        getJsonOptionLowerCase(options, "backend", backend);
        if (!backend.empty())
        {
            auto it = backendDescriptors.find(backend);
            if (it != backendDescriptors.end())
            {
                if (input.format != Format::DUMMY &&
                    suffix(input.format) != suffix(it->second))
                {
                    std::cerr << "[Warning] Supplied filename extension '"
                              << suffix(input.format)
                              << "' contradicts the backend specified via the "
                                 "'backend' key. Will go on with backend "
                              << it->first << "." << std::endl;
                }
                input.format = it->second;
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
            // WriteIterations gets the first shot at flushing
            this->m_writeIterations = std::optional<WriteIterations>();
            /*
             * Scenario: A user calls `Series::flush()` but does not check for
             * thrown exceptions. The exception will propagate further up,
             * usually thereby popping the stack frame that holds the `Series`
             * object. `Series::~Series()` will run. This check avoids that the
             * `Series` is needlessly flushed a second time. Otherwise, error
             * messages can get very confusing.
             */
            if (this->m_lastFlushSuccessful)
            {
                Series impl{{this, [](auto const *) {}}};
                impl.flush();
            }
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
} // namespace internal

Series::Series() : Attributable{nullptr}, iterations{}
{}

Series::Series(std::shared_ptr<internal::SeriesData> data)
    : Attributable{data}, m_series{std::move(data)}
{
    iterations = m_series->iterations;
}

#if openPMD_HAVE_MPI
Series::Series(
    std::string const &filepath,
    Access at,
    MPI_Comm comm,
    std::string const &options)
    : Attributable{nullptr}, m_series{new internal::SeriesData}
{
    Attributable::setData(m_series);
    iterations = m_series->iterations;
    json::TracingJSON optionsJson =
        json::parseOptions(options, comm, /* considerFiles = */ true);
    auto input = parseInput(filepath);
    parseJsonOptions(optionsJson, *input);
    auto handler =
        createIOHandler(input->path, at, input->format, comm, optionsJson);
    init(handler, std::move(input));
    json::warnGlobalUnusedOptions(optionsJson);
}
#endif

Series::Series(
    std::string const &filepath, Access at, std::string const &options)
    : Attributable{nullptr}, m_series{new internal::SeriesData}
{
    Attributable::setData(m_series);
    iterations = m_series->iterations;
    json::TracingJSON optionsJson =
        json::parseOptions(options, /* considerFiles = */ true);
    auto input = parseInput(filepath);
    parseJsonOptions(optionsJson, *input);
    auto handler = createIOHandler(input->path, at, input->format, optionsJson);
    init(handler, std::move(input));
    json::warnGlobalUnusedOptions(optionsJson);
}

Series::operator bool() const
{
    return m_series.operator bool();
}

ReadIterations Series::readIterations()
{
    // Use private constructor instead of copy constructor to avoid
    // object slicing
    return {this->m_series};
}

WriteIterations Series::writeIterations()
{
    auto &series = get();
    if (!series.m_writeIterations.has_value())
    {
        series.m_writeIterations = WriteIterations(this->iterations);
    }
    return series.m_writeIterations.value();
}

namespace
{
    std::string cleanFilename(std::string const &filename, Format f)
    {
        switch (f)
        {
        case Format::HDF5:
        case Format::ADIOS1:
        case Format::ADIOS2:
        case Format::ADIOS2_SST:
        case Format::ADIOS2_SSC:
        case Format::JSON:
            return auxiliary::replace_last(filename, suffix(f), "");
        default:
            return filename;
        }
    }

    std::function<Match(std::string const &)>
    buildMatcher(std::string const &regexPattern, int padding)
    {
        std::regex pattern(regexPattern);

        return [pattern, padding](std::string const &filename) -> Match {
            std::smatch regexMatches;
            bool match = std::regex_match(filename, regexMatches, pattern);
            int processedPadding =
                padding != 0 ? padding : (match ? regexMatches[1].length() : 0);
            return {
                match,
                processedPadding,
                match ? std::stoull(regexMatches[1]) : 0};
        };
    }

    std::function<Match(std::string const &)> matcher(
        std::string const &prefix,
        int padding,
        std::string const &postfix,
        Format f)
    {
        std::string filenameSuffix = suffix(f);
        if (filenameSuffix.empty())
        {
            return [](std::string const &) -> Match { return {false, 0, 0}; };
        }

        std::string nameReg = "^" + prefix;
        if (padding != 0)
        {
            // The part after the question mark:
            // The number must be at least `padding` digits long
            // The part before the question mark:
            // It may be longer than that only if the first digit is not zero
            // The outer pair of parentheses is for later extraction of the
            // iteration number via std::stoull(regexMatches[1])
            nameReg += "(([1-9][[:digit:]]*)?([[:digit:]]";
            nameReg += "{" + std::to_string(padding) + "}))";
        }
        else
        {
            // No padding specified, any number of digits is ok.
            nameReg += "([[:digit:]]";
            nameReg += "+)";
        }
        nameReg += postfix + filenameSuffix + "$";
        return buildMatcher(nameReg, padding);
    }
} // namespace
} // namespace openPMD
