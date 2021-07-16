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
#include "openPMD/auxiliary/Date.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/JSON.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/ReadIterations.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/version.hpp"

#include <exception>
#include <iomanip>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <tuple>


namespace openPMD
{
namespace
{
    /** Remove the filename extension of a given storage format.
     *
     * @param   filename    String containing the filename, possibly with filename extension.
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
        operator std::tuple< bool &, int &, uint64_t & >()
        {
            return std::tuple< bool &, int &, uint64_t & >{
                isContained, padding, iteration };
        }
    };

    /** Create a functor to determine if a file can be of a format and matches an iterationEncoding, given the filename on disk.
     *
     * @param   prefix      String containing head (i.e. before %T) of desired filename without filename extension.
     * @param   padding     Amount of padding allowed in iteration number %T. If zero, any amount of padding is matched.
     * @param   postfix     String containing tail (i.e. after %T) of desired filename without filename extension.
     * @param   f           File format to check backend applicability for.
     * @return  Functor returning tuple of bool and int.
     *          bool is True if file could be of type f and matches the iterationEncoding. False otherwise.
     *          int is the amount of padding present in the iteration number %T. Is 0 if bool is False.
     */
    std::function<Match(std::string const &)>
    matcher(std::string const &prefix, int padding, std::string const &postfix, Format f);
} // namespace [anonymous]

struct SeriesImpl::ParsedInput
{
    std::string path;
    std::string name;
    Format format;
    IterationEncoding iterationEncoding;
    std::string filenamePrefix;
    std::string filenamePostfix;
    int filenamePadding;
};  //ParsedInput

SeriesImpl::SeriesImpl(
    internal::SeriesData * series, internal::AttributableData * attri )
    : AttributableImpl{ attri }
    , m_series{ series }
{
}

std::string
SeriesImpl::openPMD() const
{
    return getAttribute("openPMD").get< std::string >();
}

SeriesImpl&
SeriesImpl::setOpenPMD(std::string const& o)
{
    setAttribute("openPMD", o);
    return *this;
}

uint32_t
SeriesImpl::openPMDextension() const
{
    return getAttribute("openPMDextension").get< uint32_t >();
}

SeriesImpl&
SeriesImpl::setOpenPMDextension(uint32_t oe)
{
    setAttribute("openPMDextension", oe);
    return *this;
}

std::string
SeriesImpl::basePath() const
{
    return getAttribute("basePath").get< std::string >();
}

SeriesImpl&
SeriesImpl::setBasePath(std::string const& bp)
{
    std::string version = openPMD();
    if( version == "1.0.0" || version == "1.0.1" || version == "1.1.0" )
        throw std::runtime_error("Custom basePath not allowed in openPMD <=1.1.0");

    setAttribute("basePath", bp);
    return *this;
}

std::string
SeriesImpl::meshesPath() const
{
    return getAttribute("meshesPath").get< std::string >();
}

SeriesImpl&
SeriesImpl::setMeshesPath(std::string const& mp)
{
    auto & series = get();
    if( std::any_of(series.iterations.begin(), series.iterations.end(),
                    [](Container< Iteration, uint64_t >::value_type const& i){ return i.second.meshes.written(); }) )
        throw std::runtime_error("A files meshesPath can not (yet) be changed after it has been written.");

    if( auxiliary::ends_with(mp, '/') )
        setAttribute("meshesPath", mp);
    else
        setAttribute("meshesPath", mp + "/");
    dirty() = true;
    return *this;
}

std::string
SeriesImpl::particlesPath() const
{
    return getAttribute("particlesPath").get< std::string >();
}

SeriesImpl&
SeriesImpl::setParticlesPath(std::string const& pp)
{
    auto & series = get();
    if( std::any_of(series.iterations.begin(), series.iterations.end(),
                    [](Container< Iteration, uint64_t >::value_type const& i){ return i.second.particles.written(); }) )
        throw std::runtime_error("A files particlesPath can not (yet) be changed after it has been written.");

    if( auxiliary::ends_with(pp, '/') )
        setAttribute("particlesPath", pp);
    else
        setAttribute("particlesPath", pp + "/");
    dirty() = true;
    return *this;
}

std::string
SeriesImpl::author() const
{
    return getAttribute("author").get< std::string >();
}

SeriesImpl&
SeriesImpl::setAuthor(std::string const& a)
{
    setAttribute("author", a);
    return *this;
}

std::string
SeriesImpl::software() const
{
    return getAttribute("software").get< std::string >();
}

SeriesImpl&
SeriesImpl::setSoftware( std::string const& newName, std::string const& newVersion )
{
    setAttribute( "software", newName );
    setAttribute( "softwareVersion", newVersion );
    return *this;
}

std::string
SeriesImpl::softwareVersion() const
{
    return getAttribute("softwareVersion").get< std::string >();
}

SeriesImpl&
SeriesImpl::setSoftwareVersion(std::string const& sv)
{
    setAttribute("softwareVersion", sv);
    return *this;
}

std::string
SeriesImpl::date() const
{
    return getAttribute("date").get< std::string >();
}

SeriesImpl&
SeriesImpl::setDate(std::string const& d)
{
    setAttribute("date", d);
    return *this;
}

std::string
SeriesImpl::softwareDependencies() const
{
    return getAttribute("softwareDependencies").get< std::string >();
}

SeriesImpl&
SeriesImpl::setSoftwareDependencies(std::string const &newSoftwareDependencies)
{
    setAttribute("softwareDependencies", newSoftwareDependencies);
    return *this;
}

std::string
SeriesImpl::machine() const
{
    return getAttribute("machine").get< std::string >();
}

SeriesImpl&
SeriesImpl::setMachine(std::string const &newMachine)
{
    setAttribute("machine", newMachine);
    return *this;
}

IterationEncoding
SeriesImpl::iterationEncoding() const
{
    return get().m_iterationEncoding;
}

SeriesImpl&
SeriesImpl::setIterationEncoding(IterationEncoding ie)
{
    auto & series = get();
    if( written() )
        throw std::runtime_error("A files iterationEncoding can not (yet) be changed after it has been written.");

    series.m_iterationEncoding = ie;
    switch( ie )
    {
        case IterationEncoding::fileBased:
            setIterationFormat(series.m_name);
            setAttribute("iterationEncoding", std::string("fileBased"));
            break;
        case IterationEncoding::groupBased:
            setIterationFormat(BASEPATH);
            setAttribute("iterationEncoding", std::string("groupBased"));
            break;
        case IterationEncoding::variableBased:
            setIterationFormat(
                auxiliary::replace_first(basePath(), "/%T/", ""));
            setAttribute("iterationEncoding", std::string("variableBased"));
            break;
    }
    return *this;
}

std::string
SeriesImpl::iterationFormat() const
{
    return getAttribute("iterationFormat").get< std::string >();
}

SeriesImpl&
SeriesImpl::setIterationFormat(std::string const& i)
{
    if( written() )
        throw std::runtime_error("A files iterationFormat can not (yet) be changed after it has been written.");

    if( iterationEncoding() == IterationEncoding::groupBased ||
        iterationEncoding() == IterationEncoding::variableBased )
        if( basePath() != i && (openPMD() == "1.0.1" || openPMD() == "1.0.0") )
            throw std::invalid_argument("iterationFormat must not differ from basePath " + basePath() + " for group- or variableBased data");

    setAttribute("iterationFormat", i);
    return *this;
}

std::string
SeriesImpl::name() const
{
    return get().m_name;
}

SeriesImpl&
SeriesImpl::setName(std::string const& n)
{
    auto & series = get();
    if( written() )
        throw std::runtime_error("A files name can not (yet) be changed after it has been written.");

    if( series.m_iterationEncoding == IterationEncoding::fileBased && !auxiliary::contains(series.m_name, "%T") )
            throw std::runtime_error("For fileBased formats the iteration regex %T must be included in the file name");

    series.m_name = n;
    dirty() = true;
    return *this;
}

std::string
SeriesImpl::backend() const
{
    return IOHandler()->backendName();
}

void
SeriesImpl::flush()
{
    auto & series = get();
    flush_impl(
        series.iterations.begin(),
        series.iterations.end(),
        FlushLevel::UserFlush );
}

std::unique_ptr< SeriesImpl::ParsedInput >
SeriesImpl::parseInput(std::string filepath)
{
    std::unique_ptr< SeriesImpl::ParsedInput > input{new SeriesImpl::ParsedInput};

#ifdef _WIN32
    if( auxiliary::contains(filepath, '/') )
    {
        std::cerr << "Filepaths on WINDOWS platforms may not contain slashes '/'! "
                  << "Replacing with backslashes '\\' unconditionally!" << std::endl;
        filepath = auxiliary::replace_all(filepath, "/", "\\");
    }
#else
    if( auxiliary::contains(filepath, '\\') )
    {
        std::cerr << "Filepaths on UNIX platforms may not include backslashes '\\'! "
                  << "Replacing with slashes '/' unconditionally!" << std::endl;
        filepath = auxiliary::replace_all(filepath, "\\", "/");
    }
#endif
    auto const pos = filepath.find_last_of(auxiliary::directory_separator);
    if( std::string::npos == pos )
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
    if( regexMatch.empty() )
        input->iterationEncoding = IterationEncoding::groupBased;
    else if( regexMatch.size() == 4 )
    {
        input->iterationEncoding = IterationEncoding::fileBased;
        input->filenamePrefix = regexMatch[1].str();
        std::string const& pad = regexMatch[2];
        if( pad.empty() )
            input->filenamePadding = 0;
        else
        {
            if( pad.front() != '0' )
                throw std::runtime_error("Invalid iterationEncoding " + input->name);
            input->filenamePadding = std::stoi(pad);
        }
        input->filenamePostfix = regexMatch[3].str();
    } else
        throw std::runtime_error("Can not determine iterationFormat from filename " + input->name);

    input->filenamePostfix = cleanFilename(input->filenamePostfix, input->format);

    input->name = cleanFilename(input->name, input->format);

    return input;
}

void SeriesImpl::init(
    std::shared_ptr< AbstractIOHandler > ioHandler,
    std::unique_ptr< SeriesImpl::ParsedInput > input )
{
    auto & series = get();
    writable().IOHandler = ioHandler;
    series.iterations.linkHierarchy(writable());
    series.iterations.writable().ownKeyWithinParent = { "iterations" };

    series.m_name = input->name;

    series.m_format = input->format;

    series.m_filenamePrefix = input->filenamePrefix;
    series.m_filenamePostfix = input->filenamePostfix;
    series.m_filenamePadding = input->filenamePadding;

    if(IOHandler()->m_frontendAccess == Access::READ_ONLY || IOHandler()->m_frontendAccess == Access::READ_WRITE )
    {
        /* Allow creation of values in Containers and setting of Attributes
         * Would throw for Access::READ_ONLY */
        auto oldType = IOHandler()->m_frontendAccess;
        auto newType = const_cast< Access* >(&IOHandler()->m_frontendAccess);
        *newType = Access::READ_WRITE;

        if( input->iterationEncoding == IterationEncoding::fileBased )
            readFileBased();
        else
            readGorVBased();

        if( series.iterations.empty() )
        {
            /* Access::READ_WRITE can be used to create a new Series
             * allow setting attributes in that case */
            written() = false;

            initDefaults( input->iterationEncoding );
            setIterationEncoding(input->iterationEncoding);

            written() = true;
        }

        *newType = oldType;
    } else
    {
        initDefaults( input->iterationEncoding );
        setIterationEncoding(input->iterationEncoding);
    }
}

void
SeriesImpl::initDefaults( IterationEncoding ie )
{
    if( !containsAttribute("openPMD"))
        setOpenPMD( getStandard() );
    if( !containsAttribute("openPMDextension"))
        setOpenPMDextension(0);
    if( !containsAttribute("basePath"))
    {
        if( ie == IterationEncoding::variableBased )
        {
            setAttribute(
                "basePath", auxiliary::replace_first(BASEPATH, "/%T/", ""));
        }
        else
        {
            setAttribute("basePath", std::string(BASEPATH));
        }
    }
    if( !containsAttribute("date"))
        setDate( auxiliary::getDateString() );
    if( !containsAttribute("software"))
        setSoftware( "openPMD-api", getVersion() );
}

std::future< void >
SeriesImpl::flush_impl(
    iterations_iterator begin,
    iterations_iterator end,
    FlushLevel level,
    bool flushIOHandler )
{
    IOHandler()->m_flushLevel = level;
    auto & series = get();
    series.m_lastFlushSuccessful = true;
    try
    {
        switch( iterationEncoding() )
        {
            using IE = IterationEncoding;
            case IE::fileBased:
                flushFileBased( begin, end );
                break;
            case IE::groupBased:
            case IE::variableBased:
                flushGorVBased( begin, end );
                break;
        }
        if( flushIOHandler )
        {
            auto res = IOHandler()->flush();
            IOHandler()->m_flushLevel = FlushLevel::InternalFlush;
            return res;
        }
        else
        {
            IOHandler()->m_flushLevel = FlushLevel::InternalFlush;
            return {};
        }
    }
    catch( ... )
    {
        IOHandler()->m_flushLevel = FlushLevel::InternalFlush;
        series.m_lastFlushSuccessful = false;
        throw;
    }
}

void
SeriesImpl::flushFileBased( iterations_iterator begin, iterations_iterator end )
{
    auto & series = get();
    if( end == begin )
        throw std::runtime_error(
            "fileBased output can not be written with no iterations." );

    if( IOHandler()->m_frontendAccess == Access::READ_ONLY )
        for( auto it = begin; it != end; ++it )
        {
            switch( openIterationIfDirty( it->first, it->second ) )
            {
                using IO = IterationOpened;
            case IO::RemainsClosed:
                continue;
            case IO::HasBeenOpened:
                // continue below
                break;
            }

            it->second.flush();

            if( *it->second.m_closed ==
                Iteration::CloseStatus::ClosedInFrontend )
            {
                Parameter< Operation::CLOSE_FILE > fClose;
                IOHandler()->enqueue(
                    IOTask( &it->second, std::move( fClose ) ) );
                *it->second.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }
            IOHandler()->flush();
        }
    else
    {
        bool allDirty = dirty();
        for( auto it = begin; it != end; ++it )
        {
            switch( openIterationIfDirty( it->first, it->second ) )
            {
                using IO = IterationOpened;
            case IO::RemainsClosed:
                continue;
            case IO::HasBeenOpened:
                // continue below
                break;
            }

            /* as there is only one series,
             * emulate the file belonging to each iteration as not yet written
             */
            written() = false;
            series.iterations.written() = false;

            dirty() |= it->second.dirty();
            std::string filename = iterationFilename( it->first );
            it->second.flushFileBased( filename, it->first );

            series.iterations.flush(
                auxiliary::replace_first( basePath(), "%T/", "" ) );

            flushAttributes();

            if( *it->second.m_closed ==
                Iteration::CloseStatus::ClosedInFrontend )
            {
                Parameter< Operation::CLOSE_FILE > fClose;
                IOHandler()->enqueue(
                    IOTask( &it->second, std::move( fClose ) ) );
                *it->second.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }

            IOHandler()->flush();

            /* reset the dirty bit for every iteration (i.e. file)
             * otherwise only the first iteration will have updates attributes */
            dirty() = allDirty;
        }
        dirty() = false;
    }
}

void
SeriesImpl::flushGorVBased( iterations_iterator begin, iterations_iterator end )
{
    auto & series = get();
    if( IOHandler()->m_frontendAccess == Access::READ_ONLY )
        for( auto it = begin; it != end; ++it )
        {
            switch( openIterationIfDirty( it->first, it->second ) )
            {
                using IO = IterationOpened;
            case IO::RemainsClosed:
                continue;
            case IO::HasBeenOpened:
                // continue below
                break;
            }

            it->second.flush();
            if( *it->second.m_closed ==
                Iteration::CloseStatus::ClosedInFrontend )
            {
                // the iteration has no dedicated file in group-based mode
                *it->second.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }
            IOHandler()->flush();
        }
    else
    {
        if( !written() )
        {
            Parameter< Operation::CREATE_FILE > fCreate;
            fCreate.name = series.m_name;
            fCreate.encoding = iterationEncoding();
            IOHandler()->enqueue(IOTask(this, fCreate));
        }

        series.iterations.flush(auxiliary::replace_first(basePath(), "%T/", ""));

        for( auto it = begin; it != end; ++it )
        {
            switch( openIterationIfDirty( it->first, it->second ) )
            {
                using IO = IterationOpened;
            case IO::RemainsClosed:
                continue;
            case IO::HasBeenOpened:
                // continue below
                break;
            }
            if( !it->second.written() )
            {
                it->second.parent() = getWritable( &series.iterations );
            }
            switch( iterationEncoding() )
            {
                using IE = IterationEncoding;
                case IE::groupBased:
                    it->second.flushGroupBased( it->first );
                    break;
                case IE::variableBased:
                    it->second.flushVariableBased( it->first );
                    break;
                default:
                    throw std::runtime_error(
                        "[Series] Internal control flow error" );
            }
            if( *it->second.m_closed == Iteration::CloseStatus::ClosedInFrontend )
            {
                // the iteration has no dedicated file in group-based mode
                *it->second.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }
        }

        flushAttributes();
        IOHandler()->flush();
    }
}

void
SeriesImpl::flushMeshesPath()
{
    Parameter< Operation::WRITE_ATT > aWrite;
    aWrite.name = "meshesPath";
    Attribute a = getAttribute("meshesPath");
    aWrite.resource = a.getResource();
    aWrite.dtype = a.dtype;
    IOHandler()->enqueue(IOTask(this, aWrite));
}

void
SeriesImpl::flushParticlesPath()
{
    Parameter< Operation::WRITE_ATT > aWrite;
    aWrite.name = "particlesPath";
    Attribute a = getAttribute("particlesPath");
    aWrite.resource = a.getResource();
    aWrite.dtype = a.dtype;
    IOHandler()->enqueue(IOTask(this, aWrite));
}

void
SeriesImpl::readFileBased( )
{
    auto & series = get();
    Parameter< Operation::OPEN_FILE > fOpen;
    Parameter< Operation::READ_ATT > aRead;
    fOpen.encoding = iterationEncoding();

    if( !auxiliary::directory_exists(IOHandler()->directory) )
        throw no_such_file_error("Supplied directory is not valid: " + IOHandler()->directory);

    auto isPartOfSeries = matcher(
        series.m_filenamePrefix, series.m_filenamePadding,
        series.m_filenamePostfix, series.m_format);
    bool isContained;
    int padding;
    uint64_t iterationIndex;
    std::set< int > paddings;
    for( auto const& entry : auxiliary::list_directory(IOHandler()->directory) )
    {
        std::tie(isContained, padding, iterationIndex) = isPartOfSeries(entry);
        if( isContained )
        {
            Iteration & i = series.iterations[ iterationIndex ];
            i.deferParseAccess( {
                std::to_string( iterationIndex ),
                iterationIndex,
                true,
                entry } );
            // TODO skip if the padding is exact the number of chars in an iteration?
            paddings.insert(padding);
        }
    }

    if( series.iterations.empty() )
    {
        /* Frontend access type might change during SeriesImpl::read() to allow parameter modification.
         * Backend access type stays unchanged for the lifetime of a Series. */
        if(IOHandler()->m_backendAccess == Access::READ_ONLY  )
            throw std::runtime_error("No matching iterations found: " + name());
        else
            std::cerr << "No matching iterations found: " << name() << std::endl;
    }

    auto readIterationEagerly = []( Iteration & iteration )
    {
        iteration.runDeferredParseAccess();
        Parameter< Operation::CLOSE_FILE > fClose;
        iteration.IOHandler()->enqueue( IOTask( &iteration, fClose ) );
        iteration.IOHandler()->flush();
        *iteration.m_closed = Iteration::CloseStatus::ClosedTemporarily;
    };
    if( series.m_parseLazily )
    {
        for( auto & iteration : series.iterations )
        {
            *iteration.second.m_closed =
                Iteration::CloseStatus::ParseAccessDeferred;
        }
        // open the last iteration, just to parse Series attributes
        auto getLastIteration = series.iterations.end();
        getLastIteration--;
        auto & lastIteration = getLastIteration->second;
        readIterationEagerly( lastIteration );
    }
    else
    {
        for( auto & iteration : series.iterations )
        {
            readIterationEagerly( iteration.second );
        }
    }

    if( paddings.size() == 1u )
        series.m_filenamePadding = *paddings.begin();

    /* Frontend access type might change during SeriesImpl::read() to allow parameter modification.
     * Backend access type stays unchanged for the lifetime of a Series. */
    if( paddings.size() > 1u && IOHandler()->m_backendAccess == Access::READ_WRITE )
        throw std::runtime_error("Cannot write to a series with inconsistent iteration padding. "
                                 "Please specify '%0<N>T' or open as read-only.");
}

void SeriesImpl::readOneIterationFileBased( std::string const & filePath )
{
    auto & series = get();

    Parameter< Operation::OPEN_FILE > fOpen;
    Parameter< Operation::READ_ATT > aRead;

    fOpen.name = filePath;
    IOHandler()->enqueue(IOTask(this, fOpen));
    IOHandler()->flush();
    series.iterations.parent() = getWritable(this);

    readBase();

    using DT = Datatype;
    aRead.name = "iterationEncoding";
    IOHandler()->enqueue( IOTask( this, aRead ) );
    IOHandler()->flush();
    if( *aRead.dtype == DT::STRING )
    {
        std::string encoding =
            Attribute( *aRead.resource ).get< std::string >();
        if( encoding == "fileBased" )
            series.m_iterationEncoding = IterationEncoding::fileBased;
        else if( encoding == "groupBased" )
        {
            series.m_iterationEncoding = IterationEncoding::groupBased;
            std::cerr << "Series constructor called with iteration "
                            "regex '%T' suggests loading a "
                        << "time series with fileBased iteration "
                            "encoding. Loaded file is groupBased.\n";
        }
        else if( encoding == "variableBased" )
        {
            /*
             * Unlike if the file were group-based, this one doesn't work
             * at all since the group paths are different.
             */
            throw std::runtime_error(
                "Series constructor called with iteration "
                "regex '%T' suggests loading a "
                "time series with fileBased iteration "
                "encoding. Loaded file is variableBased." );
        }
        else
            throw std::runtime_error(
                "Unknown iterationEncoding: " + encoding );
        setAttribute( "iterationEncoding", encoding );
    }
    else
        throw std::runtime_error( "Unexpected Attribute datatype "
                                    "for 'iterationEncoding'" );

    aRead.name = "iterationFormat";
    IOHandler()->enqueue( IOTask( this, aRead ) );
    IOHandler()->flush();
    if( *aRead.dtype == DT::STRING )
    {
        written() = false;
        setIterationFormat(
            Attribute( *aRead.resource ).get< std::string >() );
        written() = true;
    }
    else
        throw std::runtime_error(
            "Unexpected Attribute datatype for 'iterationFormat'" );

    Parameter< Operation::OPEN_PATH > pOpen;
    std::string version = openPMD();
    if( version == "1.0.0" || version == "1.0.1" || version == "1.1.0" )
        pOpen.path = auxiliary::replace_first(basePath(), "/%T/", "");
    else
        throw std::runtime_error("Unknown openPMD version - " + version);
    IOHandler()->enqueue(IOTask(&series.iterations, pOpen));

    readAttributes( ReadMode::IgnoreExisting );
    series.iterations.readAttributes(ReadMode::OverrideExisting );
}

void
SeriesImpl::readGorVBased( bool do_init )
{
    auto & series = get();
    Parameter< Operation::OPEN_FILE > fOpen;
    fOpen.name = series.m_name;
    fOpen.encoding = iterationEncoding();
    IOHandler()->enqueue(IOTask(this, fOpen));
    IOHandler()->flush();

    if( do_init )
    {
        readBase();

        using DT = Datatype;
        Parameter< Operation::READ_ATT > aRead;
        aRead.name = "iterationEncoding";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush();
        if( *aRead.dtype == DT::STRING )
        {
            std::string encoding = Attribute(*aRead.resource).get< std::string >();
            if( encoding == "groupBased" )
                series.m_iterationEncoding = IterationEncoding::groupBased;
            else if( encoding == "variableBased" )
                series.m_iterationEncoding = IterationEncoding::variableBased;
            else if( encoding == "fileBased" )
            {
                series.m_iterationEncoding = IterationEncoding::fileBased;
                std::cerr << "Series constructor called with explicit iteration suggests loading a "
                          << "single file with groupBased iteration encoding. Loaded file is fileBased.\n";
                /*
                 * We'll want the openPMD API to continue series.m_name to open
                 * the file instead of piecing the name together via
                 * prefix-padding-postfix things.
                 */
                series.m_overrideFilebasedFilename = series.m_name;
            } else
                throw std::runtime_error("Unknown iterationEncoding: " + encoding);
            setAttribute("iterationEncoding", encoding);
        }
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'iterationEncoding'");

        aRead.name = "iterationFormat";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush();
        if( *aRead.dtype == DT::STRING )
        {
            written() = false;
            setIterationFormat(Attribute(*aRead.resource).get< std::string >());
            written() = true;
        }
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'iterationFormat'");
    }

    Parameter< Operation::OPEN_PATH > pOpen;
    std::string version = openPMD();
    if( version == "1.0.0" || version == "1.0.1" || version == "1.1.0" )
        pOpen.path = auxiliary::replace_first(basePath(), "/%T/", "");
    else
        throw std::runtime_error("Unknown openPMD version - " + version);
    IOHandler()->enqueue(IOTask(&series.iterations, pOpen));

    readAttributes( ReadMode::IgnoreExisting );
    /*
     * 'snapshot' changes over steps, so reread that.
     */
    series.iterations.readAttributes( ReadMode::OverrideExisting );
    /* obtain all paths inside the basepath (i.e. all iterations) */
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler()->enqueue(IOTask(&series.iterations, pList));
    IOHandler()->flush();

    auto readSingleIteration =
        [&series, &pOpen, this]
        (uint64_t index, std::string path, bool guardClosed )
    {
        if( series.iterations.contains( index ) )
        {
            // maybe re-read
            auto & i = series.iterations.at( index );
            if( guardClosed && i.closedByWriter() )
            {
                return;
            }
            if( *i.m_closed != Iteration::CloseStatus::ParseAccessDeferred )
            {
                pOpen.path = path;
                IOHandler()->enqueue( IOTask( &i, pOpen ) );
                i.reread( path );
            }
        }
        else
        {
            // parse for the first time, resp. delay the parsing process
            Iteration & i = series.iterations[ index ];
            i.deferParseAccess( { path, index, false, "" } );
            if( !series.m_parseLazily )
            {
                i.runDeferredParseAccess();
                *i.m_closed = Iteration::CloseStatus::Open;
            }
            else
            {
                *i.m_closed = Iteration::CloseStatus::ParseAccessDeferred;
            }
        }
    };

    switch( iterationEncoding() )
    {
    case IterationEncoding::groupBased:
    /*
     * Sic! This happens when a file-based Series is opened in group-based mode.
     */
    case IterationEncoding::fileBased:
        for( auto const & it : *pList.paths )
        {
            uint64_t index = std::stoull( it );
            readSingleIteration( index, it, true );
        }
        break;
    case IterationEncoding::variableBased:
    {
        uint64_t index = 0;
        if( series.iterations.containsAttribute( "snapshot" ) )
        {
            index = series.iterations
                .getAttribute( "snapshot" )
                .get< uint64_t >();
        }
        readSingleIteration( index, "", false );
        break;
    }
    }
}

void
SeriesImpl::readBase()
{
    auto & series = get();
    using DT = Datatype;
    Parameter< Operation::READ_ATT > aRead;

    aRead.name = "openPMD";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush();
    if( *aRead.dtype == DT::STRING )
        setOpenPMD(Attribute(*aRead.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'openPMD'");

    aRead.name = "openPMDextension";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush();
    if( *aRead.dtype == determineDatatype< uint32_t >() )
        setOpenPMDextension(Attribute(*aRead.resource).get< uint32_t >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'openPMDextension'");

    aRead.name = "basePath";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush();
    if( *aRead.dtype == DT::STRING )
        setAttribute("basePath", Attribute(*aRead.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'basePath'");

    Parameter< Operation::LIST_ATTS > aList;
    IOHandler()->enqueue(IOTask(this, aList));
    IOHandler()->flush();
    if( std::count(aList.attributes->begin(), aList.attributes->end(), "meshesPath") == 1 )
    {
        aRead.name = "meshesPath";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush();
        if( *aRead.dtype == DT::STRING )
        {
            /* allow setting the meshes path after completed IO */
            for( auto& it : series.iterations )
                it.second.meshes.written() = false;

            setMeshesPath(Attribute(*aRead.resource).get< std::string >());

            for( auto& it : series.iterations )
                it.second.meshes.written() = true;
        }
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'meshesPath'");
    }

    if( std::count(aList.attributes->begin(), aList.attributes->end(), "particlesPath") == 1 )
    {
        aRead.name = "particlesPath";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush();
        if( *aRead.dtype == DT::STRING )
        {
            /* allow setting the meshes path after completed IO */
            for( auto& it : series.iterations )
                it.second.particles.written() = false;

            setParticlesPath(Attribute(*aRead.resource).get< std::string >());


            for( auto& it : series.iterations )
                it.second.particles.written() = true;
        }
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'particlesPath'");
    }
}

std::string
SeriesImpl::iterationFilename( uint64_t i )
{
    auto & series = get();
    if( series.m_overrideFilebasedFilename.has_value() )
    {
        return series.m_overrideFilebasedFilename.get();
    }
    std::stringstream iteration( "" );
    iteration << std::setw( series.m_filenamePadding )
              << std::setfill( '0' ) << i;
    return series.m_filenamePrefix + iteration.str()
           + series.m_filenamePostfix;
}

SeriesImpl::iterations_iterator
SeriesImpl::indexOf( Iteration const & iteration )
{
    auto & series = get();
    for( auto it = series.iterations.begin(); it != series.iterations.end();
         ++it )
    {
        if( &it->second.Attributable::get() == &iteration.Attributable::get() )
        {
            return it;
        }
    }
    throw std::runtime_error(
        "[Iteration::close] Iteration not found in Series." );
}

AdvanceStatus
SeriesImpl::advance(
    AdvanceMode mode,
    internal::AttributableData & file,
    iterations_iterator begin,
    Iteration & iteration )
{
    auto & series = get();
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
    Iteration::CloseStatus oldCloseStatus = *iteration.m_closed;
    if( oldCloseStatus == Iteration::CloseStatus::ClosedInFrontend )
    {
        *iteration.m_closed = Iteration::CloseStatus::Open;
    }

    flush_impl(
        begin, end, FlushLevel::UserFlush, /* flushIOHandler = */ false );

    if( oldCloseStatus == Iteration::CloseStatus::ClosedInFrontend )
    {
        // Series::flush() would normally turn a `ClosedInFrontend` into
        // a `ClosedInBackend`. Do that manually.
        *iteration.m_closed = Iteration::CloseStatus::ClosedInBackend;
    }
    else if(
        oldCloseStatus == Iteration::CloseStatus::ClosedInBackend &&
        series.m_iterationEncoding == IterationEncoding::fileBased )
    {
        /*
         * In file-based iteration encoding, we want to avoid accidentally
         * opening an iteration's file by beginning a step on it.
         * So, return now.
         */
        return AdvanceStatus::OK;
    }

    Parameter< Operation::ADVANCE > param;
    if( *iteration.m_closed == Iteration::CloseStatus::ClosedTemporarily &&
        series.m_iterationEncoding == IterationEncoding::fileBased )
    {
        /*
         * If the Series has file-based iteration layout and the file has not
         * been opened by flushFileFileBased(), there's no use in nagging the
         * backend to do anything.
         */
        param.status = std::make_shared< AdvanceStatus >( AdvanceStatus::OK );
    }
    else
    {
        param.mode = mode;
        IOTask task( &file.m_writable, param );
        IOHandler()->enqueue( task );
    }


    if( oldCloseStatus == Iteration::CloseStatus::ClosedInFrontend &&
        mode == AdvanceMode::ENDSTEP )
    {
        using IE = IterationEncoding;
        switch( series.m_iterationEncoding )
        {
            case IE::fileBased:
            {
                if( *iteration.m_closed !=
                    Iteration::CloseStatus::ClosedTemporarily )
                {
                    Parameter< Operation::CLOSE_FILE > fClose;
                    IOHandler()->enqueue(
                        IOTask( &iteration, std::move( fClose ) ) );
                }
                *iteration.m_closed = Iteration::CloseStatus::ClosedInBackend;
                break;
            }
            case IE::groupBased:
            {
                // We can now put some groups to rest
                Parameter< Operation::CLOSE_PATH > fClose;
                IOHandler()->enqueue( IOTask( &iteration, std::move( fClose ) ) );
                // In group-based iteration layout, files are
                // not closed on a per-iteration basis
                // We will treat it as such nonetheless
                *iteration.m_closed = Iteration::CloseStatus::ClosedInBackend;
                break;
            }
            case IE::variableBased: // no action necessary
                break;
        }
    }

    // We cannot call SeriesImpl::flush now, since the IO handler is still filled
    // from calling flush(Group|File)based, but has not been emptied yet
    // Do that manually
    IOHandler()->m_flushLevel = FlushLevel::UserFlush;
    try
    {
        IOHandler()->flush();
    }
    catch( ... )
    {
        IOHandler()->m_flushLevel = FlushLevel::InternalFlush;
        throw;
    }
    IOHandler()->m_flushLevel = FlushLevel::InternalFlush;

    return *param.status;
}

auto SeriesImpl::openIterationIfDirty( uint64_t index, Iteration iteration )
    -> IterationOpened
{
    /*
     * Check side conditions on accessing iterations, and if they are fulfilled,
     * forward function params to openIteration().
     */
    if( *iteration.m_closed == Iteration::CloseStatus::ParseAccessDeferred )
    {
        return IterationOpened::RemainsClosed;
    }
    bool const dirtyRecursive = iteration.dirtyRecursive();
    if( *iteration.m_closed == Iteration::CloseStatus::ClosedInBackend )
    {
        // file corresponding with the iteration has previously been
        // closed and fully flushed
        // verify that there have been no further accesses
        if( !iteration.written() )
        {
            throw std::runtime_error(
                "[Series] Closed iteration has not been written. This "
                "is an internal error." );
        }
        if( dirtyRecursive )
        {
            throw std::runtime_error(
                "[Series] Detected illegal access to iteration that "
                "has been closed previously." );
        }
        return IterationOpened::RemainsClosed;
    }

    switch( iterationEncoding() )
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
        if( dirtyRecursive || this->dirty() )
        {
            // openIteration() will update the close status
            openIteration( index, iteration );
            return IterationOpened::HasBeenOpened;
        }
        break;
    case IE::groupBased:
    case IE::variableBased:
        // open unconditionally
        // this makes groupBased encoding safer for parallel usage
        // (variable-based encoding runs in lockstep anyway)
        // openIteration() will update the close status
        openIteration( index, iteration );
        return IterationOpened::HasBeenOpened;
    }
    return IterationOpened::RemainsClosed;
}

void SeriesImpl::openIteration( uint64_t index, Iteration iteration )
{
    auto oldStatus = *iteration.m_closed;
    switch( *iteration.m_closed )
    {
        using CL = Iteration::CloseStatus;
    case CL::ClosedInBackend:
        throw std::runtime_error(
            "[Series] Detected illegal access to iteration that "
            "has been closed previously." );
    case CL::ParseAccessDeferred:
    case CL::Open:
    case CL::ClosedTemporarily:
        *iteration.m_closed = CL::Open;
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
    switch( iterationEncoding() )
    {
        using IE = IterationEncoding;
    case IE::fileBased: {
        switch( IOHandler()->m_frontendAccess )
        {
        case Access::READ_ONLY:
        case Access::READ_WRITE: {
            /*
             * The iteration is marked written() as soon as its file has been
             * either created or opened.
             * If the iteration has not been created yet, it cannot be opened.
             * In that case, it is not written() and its old close status was
             * not ParseAccessDeferred.
             */
            if( !iteration.written() &&
                oldStatus != Iteration::CloseStatus::ParseAccessDeferred )
            {
                // nothing to do, file will be opened by writing routines
                break;
            }
            auto & series = get();
            // open the iteration's file again
            Parameter< Operation::OPEN_FILE > fOpen;
            fOpen.encoding = iterationEncoding();
            fOpen.name = iterationFilename( index );
            IOHandler()->enqueue( IOTask( this, fOpen ) );

            /* open base path */
            Parameter< Operation::OPEN_PATH > pOpen;
            pOpen.path = auxiliary::replace_first( basePath(), "%T/", "" );
            IOHandler()->enqueue( IOTask( &series.iterations, pOpen ) );
            /* open iteration path */
            pOpen.path = iterationEncoding() == IterationEncoding::variableBased
                ? ""
                : std::to_string( index );
            IOHandler()->enqueue( IOTask( &iteration, pOpen ) );
            break;
        }
        case Access::CREATE:
            // nothing to do, file will be opened by writing routines
            break;
        }
    }
    case IE::groupBased:
    case IE::variableBased:
        // nothing to do, no opening necessary in those modes
        break;
    }
}

namespace
{
template< typename T >
void getJsonOption(
    nlohmann::json const & config, std::string const & key, T & dest )
{
    if( config.contains( key ) )
    {
        dest = config.at( key ).get< T >();
    }
}

void parseJsonOptions(
    internal::SeriesData & series, nlohmann::json const & options )
{
    getJsonOption( options, "defer_iteration_parsing", series.m_parseLazily );
}
}

namespace internal
{
#if openPMD_HAVE_MPI
SeriesInternal::SeriesInternal(
    std::string const & filepath,
    Access at,
    MPI_Comm comm,
    std::string const & options )
    : SeriesImpl{
          static_cast< internal::SeriesData * >( this ),
          static_cast< internal::AttributableData * >( this ) }
{
    nlohmann::json optionsJson = auxiliary::parseOptions( options, comm );
    parseJsonOptions( *this, optionsJson );
    auto input = parseInput( filepath );
    auto handler = createIOHandler(
        input->path, at, input->format, comm, std::move( optionsJson ) );
    init( handler, std::move( input ) );
}
#endif

SeriesInternal::SeriesInternal(
    std::string const & filepath, Access at, std::string const & options )
    : SeriesImpl{
          static_cast< internal::SeriesData * >( this ),
          static_cast< internal::AttributableData * >( this ) }
{
    nlohmann::json optionsJson = auxiliary::parseOptions( options );
    parseJsonOptions( *this, optionsJson );
    auto input = parseInput( filepath );
    auto handler = createIOHandler(
        input->path, at, input->format, std::move( optionsJson ) );
    init( handler, std::move( input ) );
}

SeriesInternal::~SeriesInternal()
{
    // we must not throw in a destructor
    try
    {
        auto & series = get();
        // WriteIterations gets the first shot at flushing
        series.m_writeIterations = auxiliary::Option< WriteIterations >();
        /*
         * Scenario: A user calls `Series::flush()` but does not check for
         * thrown exceptions. The exception will propagate further up, usually
         * thereby popping the stack frame that holds the `Series` object.
         * `Series::~Series()` will run. This check avoids that the `Series` is
         * needlessly flushed a second time. Otherwise, error messages can get
         * very confusing.
         */
        if( get().m_lastFlushSuccessful )
        {
            flush();
        }
    }
    catch( std::exception const & ex )
    {
        std::cerr << "[~Series] An error occurred: " << ex.what() << std::endl;
    }
    catch( ... )
    {
        std::cerr << "[~Series] An error occurred." << std::endl;
    }
}
} // namespace internal

Series::Series() : SeriesImpl{ nullptr, nullptr }, iterations{}
{
}

#if openPMD_HAVE_MPI
Series::Series(
    std::string const & filepath,
    Access at,
    MPI_Comm comm,
    std::string const & options )
    : SeriesImpl{ nullptr, nullptr }
    , m_series{ std::make_shared< internal::SeriesInternal >(
          filepath, at, comm, options ) }
    , iterations{ m_series->iterations }
{
    AttributableImpl::m_attri =
        static_cast< internal::AttributableData * >( m_series.get() );
    SeriesImpl::m_series = m_series.get();
}
#endif

Series::Series(
    std::string const & filepath,
    Access at,
    std::string const & options)
    : SeriesImpl{ nullptr, nullptr }
    , m_series{ std::make_shared< internal::SeriesInternal >(
          filepath, at, options ) }
    , iterations{ m_series->iterations }
{
    AttributableImpl::m_attri =
        static_cast< internal::AttributableData * >( m_series.get() );
    SeriesImpl::m_series = m_series.get();
}

Series::operator bool() const
{
    return m_series.operator bool();
}

ReadIterations Series::readIterations()
{
    return { *this };
}

WriteIterations
Series::writeIterations()
{
    auto & series = get();
    if( !series.m_writeIterations.has_value() )
    {
        series.m_writeIterations = WriteIterations( this->iterations );
    }
    return series.m_writeIterations.get();
}

namespace
{
    std::string
    cleanFilename(std::string const &filename, Format f) {
        switch (f) {
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
    buildMatcher(std::string const &regexPattern) {
        std::regex pattern(regexPattern);

        return [pattern](std::string const &filename) -> Match {
            std::smatch regexMatches;
            bool match = std::regex_match(filename, regexMatches, pattern);
            int padding = match ? regexMatches[1].length() : 0;
            return {match, padding, match ? std::stoull(regexMatches[1]) : 0};
        };
    }

    std::function<Match(std::string const &)>
    matcher(std::string const &prefix, int padding, std::string const &postfix, Format f) {
        switch (f) {
            case Format::HDF5: {
                std::string nameReg = "^" + prefix + "([[:digit:]]";
                if (padding != 0)
                    nameReg += "{" + std::to_string(padding) + "}";
                else
                    nameReg += "+";
                nameReg += +")" + postfix + ".h5$";
                return buildMatcher(nameReg);
            }
            case Format::ADIOS1:
            case Format::ADIOS2: {
                std::string nameReg = "^" + prefix + "([[:digit:]]";
                if (padding != 0)
                    nameReg += "{" + std::to_string(padding) + "}";
                else
                    nameReg += "+";
                nameReg += +")" + postfix + ".bp$";
                return buildMatcher(nameReg);
            }
            case Format::ADIOS2_SST:
            {
                std::string nameReg = "^" + prefix + "([[:digit:]]";
                if( padding != 0 )
                    nameReg += "{" + std::to_string(padding) + "}";
                else
                    nameReg += "+";
                nameReg += + ")" + postfix + ".sst$";
                return buildMatcher(nameReg);
            }
            case Format::ADIOS2_SSC:
            {
                std::string nameReg = "^" + prefix + "([[:digit:]]";
                if( padding != 0 )
                    nameReg += "{" + std::to_string(padding) + "}";
                else
                    nameReg += "+";
                nameReg += + ")" + postfix + ".ssc$";
                return buildMatcher(nameReg);
            }
            case Format::JSON: {
                std::string nameReg = "^" + prefix + "([[:digit:]]";
                if (padding != 0)
                    nameReg += "{" + std::to_string(padding) + "}";
                else
                    nameReg += "+";
                nameReg += +")" + postfix + ".json$";
                return buildMatcher(nameReg);
            }
            default:
                return [](std::string const &) -> Match { return {false, 0, 0}; };
        }
    }
} // namespace [anonymous]
} // namespace openPMD
