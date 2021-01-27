/* Copyright 2017-2020 Fabian Koller, Axel Huebl
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
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"
#include "openPMD/IO/Format.hpp"
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
    std::function<std::tuple<bool, int>(std::string const &)>
    matcher(std::string const &prefix, int padding, std::string const &postfix, Format f);
} // namespace [anonymous]

struct Series::ParsedInput
{
    std::string path;
    std::string name;
    Format format;
    IterationEncoding iterationEncoding;
    std::string filenamePrefix;
    std::string filenamePostfix;
    int filenamePadding;
};  //ParsedInput

#if openPMD_HAVE_MPI
Series::Series(
    std::string const & filepath,
    Access at,
    MPI_Comm comm,
    std::string const & options )
    : iterations{ Container< Iteration, uint64_t >() }
    , m_iterationEncoding{ std::make_shared< IterationEncoding >() }
{
    auto input = parseInput( filepath );
    auto handler =
        createIOHandler( input->path, at, input->format, comm, options );
    init( handler, std::move( input ) );
}
#endif

Series::Series(
    std::string const & filepath,
    Access at,
    std::string const & options )
    : iterations{ Container< Iteration, uint64_t >() }
    , m_iterationEncoding{ std::make_shared< IterationEncoding >() }
{
    auto input = parseInput( filepath );
    auto handler = createIOHandler( input->path, at, input->format, options );
    init(handler, std::move(input));
}

Series::~Series()
{
    // we must not throw in a destructor
    try
    {
        flush();
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

std::string
Series::openPMD() const
{
    return getAttribute("openPMD").get< std::string >();
}

Series&
Series::setOpenPMD(std::string const& o)
{
    setAttribute("openPMD", o);
    return *this;
}

uint32_t
Series::openPMDextension() const
{
    return getAttribute("openPMDextension").get< uint32_t >();
}

Series&
Series::setOpenPMDextension(uint32_t oe)
{
    setAttribute("openPMDextension", oe);
    return *this;
}

std::string
Series::basePath() const
{
    return getAttribute("basePath").get< std::string >();
}

Series&
Series::setBasePath(std::string const& bp)
{
    std::string version = openPMD();
    if( version == "1.0.0" || version == "1.0.1" || version == "1.1.0" )
        throw std::runtime_error("Custom basePath not allowed in openPMD <=1.1.0");

    setAttribute("basePath", bp);
    return *this;
}

std::string
Series::meshesPath() const
{
    return getAttribute("meshesPath").get< std::string >();
}

Series&
Series::setMeshesPath(std::string const& mp)
{
    if( std::any_of(iterations.begin(), iterations.end(),
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
Series::particlesPath() const
{
    return getAttribute("particlesPath").get< std::string >();
}

Series&
Series::setParticlesPath(std::string const& pp)
{
    if( std::any_of(iterations.begin(), iterations.end(),
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
Series::author() const
{
    return getAttribute("author").get< std::string >();
}

Series&
Series::setAuthor(std::string const& a)
{
    setAttribute("author", a);
    return *this;
}

std::string
Series::software() const
{
    return getAttribute("software").get< std::string >();
}

Series&
Series::setSoftware( std::string const& newName, std::string const& newVersion )
{
    setAttribute( "software", newName );
    setAttribute( "softwareVersion", newVersion );
    return *this;
}

std::string
Series::softwareVersion() const
{
    return getAttribute("softwareVersion").get< std::string >();
}

Series&
Series::setSoftwareVersion(std::string const& sv)
{
    setAttribute("softwareVersion", sv);
    return *this;
}

std::string
Series::date() const
{
    return getAttribute("date").get< std::string >();
}

Series&
Series::setDate(std::string const& d)
{
    setAttribute("date", d);
    return *this;
}

std::string
Series::softwareDependencies() const
{
    return getAttribute("softwareDependencies").get< std::string >();
}

Series&
Series::setSoftwareDependencies(std::string const &newSoftwareDependencies)
{
    setAttribute("softwareDependencies", newSoftwareDependencies);
    return *this;
}

std::string
Series::machine() const
{
    return getAttribute("machine").get< std::string >();
}

Series&
Series::setMachine(std::string const &newMachine)
{
    setAttribute("machine", newMachine);
    return *this;
}

IterationEncoding
Series::iterationEncoding() const
{
    return *m_iterationEncoding;
}

Series&
Series::setIterationEncoding(IterationEncoding ie)
{
    if( written() )
        throw std::runtime_error("A files iterationEncoding can not (yet) be changed after it has been written.");

    *m_iterationEncoding = ie;
    switch( ie )
    {
        case IterationEncoding::fileBased:
            setIterationFormat(*m_name);
            setAttribute("iterationEncoding", std::string("fileBased"));
            break;
        case IterationEncoding::groupBased:
            setIterationFormat(BASEPATH);
            setAttribute("iterationEncoding", std::string("groupBased"));
            break;
    }
    return *this;
}

std::string
Series::iterationFormat() const
{
    return getAttribute("iterationFormat").get< std::string >();
}

Series&
Series::setIterationFormat(std::string const& i)
{
    if( written() )
        throw std::runtime_error("A files iterationFormat can not (yet) be changed after it has been written.");

    if( *m_iterationEncoding == IterationEncoding::groupBased )
        if( basePath() != i && (openPMD() == "1.0.1" || openPMD() == "1.0.0") )
            throw std::invalid_argument("iterationFormat must not differ from basePath " + basePath() + " for groupBased data");

    setAttribute("iterationFormat", i);
    return *this;
}

std::string
Series::name() const
{
    return *m_name;
}

Series&
Series::setName(std::string const& n)
{
    if( written() )
        throw std::runtime_error("A files name can not (yet) be changed after it has been written.");

    if( *m_iterationEncoding == IterationEncoding::fileBased && !auxiliary::contains(*m_name, "%T") )
            throw std::runtime_error("For fileBased formats the iteration regex %T must be included in the file name");

    *m_name = n;
    dirty() = true;
    return *this;
}

std::string
Series::backend() const
{
    return IOHandler->backendName();
}

void
Series::flush()
{
    flush_impl( iterations.begin(), iterations.end() );
}

ReadIterations
Series::readIterations()
{
    return { this };
}

WriteIterations
Series::writeIterations()
{
    if( !m_writeIterations->has_value() )
    {
        *m_writeIterations = WriteIterations( this->iterations );
    }
    return m_writeIterations->get();
}

std::unique_ptr< Series::ParsedInput >
Series::parseInput(std::string filepath)
{
    std::unique_ptr< Series::ParsedInput > input{new Series::ParsedInput};

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

void
Series::init(std::shared_ptr< AbstractIOHandler > ioHandler,
             std::unique_ptr< Series::ParsedInput > input)
{
    m_writable->IOHandler = ioHandler;
    IOHandler = m_writable->IOHandler.get();
    iterations.linkHierarchy(m_writable);

    m_name = std::make_shared< std::string >(input->name);

    m_format = std::make_shared< Format >(input->format);

    m_filenamePrefix = std::make_shared< std::string >(input->filenamePrefix);
    m_filenamePostfix = std::make_shared< std::string >(input->filenamePostfix);
    m_filenamePadding = std::make_shared< int >(input->filenamePadding);

    if(IOHandler->m_frontendAccess == Access::READ_ONLY || IOHandler->m_frontendAccess == Access::READ_WRITE )
    {
        /* Allow creation of values in Containers and setting of Attributes
         * Would throw for Access::READ_ONLY */
        auto oldType = IOHandler->m_frontendAccess;
        auto newType = const_cast< Access* >(&m_writable->IOHandler->m_frontendAccess);
        *newType = Access::READ_WRITE;

        if( input->iterationEncoding == IterationEncoding::fileBased )
            readFileBased();
        else
            readGroupBased();

        if( iterations.empty() )
        {
            /* Access::READ_WRITE can be used to create a new Series
             * allow setting attributes in that case */
            written() = false;

            initDefaults();
            setIterationEncoding(input->iterationEncoding);

            written() = true;
        }

        *newType = oldType;
    } else
    {
        initDefaults();
        setIterationEncoding(input->iterationEncoding);
    }
}

void
Series::initDefaults()
{
    if( !containsAttribute("openPMD"))
        setOpenPMD( getStandard() );
    if( !containsAttribute("openPMDextension"))
        setOpenPMDextension(0);
    if( !containsAttribute("basePath"))
        setAttribute("basePath", std::string(BASEPATH));
    if( !containsAttribute("date"))
        setDate( auxiliary::getDateString() );
    if( !containsAttribute("software"))
        setSoftware( "openPMD-api", getVersion() );
}

std::future< void >
Series::flush_impl( iterations_iterator begin, iterations_iterator end )
{
    switch( *m_iterationEncoding )
    {
        using IE = IterationEncoding;
        case IE::fileBased:
            flushFileBased( begin, end );
            break;
        case IE::groupBased:
            flushGroupBased( begin, end );
            break;
    }

    return IOHandler->flush();
}

void
Series::flushFileBased( iterations_iterator begin, iterations_iterator end )
{
    if( end == begin )
        throw std::runtime_error(
            "fileBased output can not be written with no iterations." );

    if( IOHandler->m_frontendAccess == Access::READ_ONLY )
        for( auto it = begin; it != end; ++it )
        {
            bool const dirtyRecursive = it->second.dirtyRecursive();
            if( *it->second.m_closed
                == Iteration::CloseStatus::ClosedInBackend )
            {
                // file corresponding with the iteration has previously been
                // closed and fully flushed
                // verify that there have been no further accesses
                if( dirtyRecursive )
                {
                    throw std::runtime_error(
                        "[Series] Detected illegal access to iteration that "
                        "has been closed previously." );
                }
                continue;
            }
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
                openIteration( it->first, it->second );
                it->second.flush();
            }

            if( *it->second.m_closed
                == Iteration::CloseStatus::ClosedInFrontend )
            {
                Parameter< Operation::CLOSE_FILE > fClose;
                IOHandler->enqueue(
                    IOTask( &it->second, std::move( fClose ) ) );
                *it->second.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }
            IOHandler->flush();
        }
    else
    {
        bool allDirty = dirty();
        for( auto it = begin; it != end; ++it )
        {
            bool const dirtyRecursive = it->second.dirtyRecursive();
            if( *it->second.m_closed
                == Iteration::CloseStatus::ClosedInBackend )
            {
                // file corresponding with the iteration has previously been
                // closed and fully flushed
                // verify that there have been no further accesses
                if (!it->second.written())
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
                continue;
            }

            /*
             * Opening a file is expensive, so let's do it only if necessary.
             * Necessary if:
             * 1. The iteration itself has been changed somewhere.
             * 2. Or the Series has been changed globally in a manner that
             *    requires adapting all iterations.
             */
            if( dirtyRecursive || this->dirty() )
            {
                /* as there is only one series,
                * emulate the file belonging to each iteration as not yet written
                */
                written() = false;
                iterations.written() = false;

                dirty() |= it->second.dirty();
                std::string filename = iterationFilename( it->first );
                it->second.flushFileBased(filename, it->first);

                iterations.flush(
                    auxiliary::replace_first(basePath(), "%T/", ""));

                flushAttributes();

                switch( *it->second.m_closed )
                {
                    using CL = Iteration::CloseStatus;
                    case CL::Open:
                    case CL::ClosedTemporarily:
                        *it->second.m_closed = CL::Open;
                        break;
                    default:
                        // keep it
                        break;
                }
            }

            if( *it->second.m_closed ==
                Iteration::CloseStatus::ClosedInFrontend )
            {
                Parameter< Operation::CLOSE_FILE > fClose;
                IOHandler->enqueue(
                    IOTask( &it->second, std::move( fClose ) ) );
                *it->second.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }

            IOHandler->flush();

            /* reset the dirty bit for every iteration (i.e. file)
             * otherwise only the first iteration will have updates attributes */
            dirty() = allDirty;
        }
        dirty() = false;
    }
}

void
Series::flushGroupBased( iterations_iterator begin, iterations_iterator end )
{
    if( IOHandler->m_frontendAccess == Access::READ_ONLY )
        for( auto it = begin; it != end; ++it )
        {
            if( *it->second.m_closed ==
                Iteration::CloseStatus::ClosedInBackend )
            {
                // file corresponding with the iteration has previously been
                // closed and fully flushed
                // verify that there have been no further accesses
                if( it->second.dirtyRecursive() )
                {
                    throw std::runtime_error(
                        "[Series] Illegal access to iteration " +
                        std::to_string( it->first ) +
                        " that has been closed previously." );
                }
                continue;
            }
            it->second.flush();
            if( *it->second.m_closed == Iteration::CloseStatus::ClosedInFrontend )
            {
                // the iteration has no dedicated file in group-based mode
                *it->second.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }
            IOHandler->flush();
        }
    else
    {
        if( !written() )
        {
            Parameter< Operation::CREATE_FILE > fCreate;
            fCreate.name = *m_name;
            IOHandler->enqueue(IOTask(this, fCreate));
        }

        iterations.flush(auxiliary::replace_first(basePath(), "%T/", ""));

        for( auto it = begin; it != end; ++it )
        {
            if( *it->second.m_closed ==
                Iteration::CloseStatus::ClosedInBackend )
            {
                // file corresponding with the iteration has previously been
                // closed and fully flushed
                // verify that there have been no further accesses
                if (!it->second.written())
                {
                    throw std::runtime_error(
                        "[Series] Closed iteration has not been written. This "
                        "is an internal error." );
                }
                if( it->second.dirtyRecursive() )
                {
                    throw std::runtime_error(
                        "[Series] Illegal access to iteration " +
                        std::to_string( it->first ) +
                        " that has been closed previously." );
                }
                continue;
            }
            if( !it->second.written() )
            {
                it->second.m_writable->parent = getWritable( &iterations );
                it->second.parent = getWritable( &iterations );
            }
            it->second.flushGroupBased(it->first);
            if( *it->second.m_closed == Iteration::CloseStatus::ClosedInFrontend )
            {
                // the iteration has no dedicated file in group-based mode
                *it->second.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }
        }

        flushAttributes();
        IOHandler->flush();
    }
}

void
Series::flushMeshesPath()
{
    Parameter< Operation::WRITE_ATT > aWrite;
    aWrite.name = "meshesPath";
    Attribute a = getAttribute("meshesPath");
    aWrite.resource = a.getResource();
    aWrite.dtype = a.dtype;
    IOHandler->enqueue(IOTask(this, aWrite));
}

void
Series::flushParticlesPath()
{
    Parameter< Operation::WRITE_ATT > aWrite;
    aWrite.name = "particlesPath";
    Attribute a = getAttribute("particlesPath");
    aWrite.resource = a.getResource();
    aWrite.dtype = a.dtype;
    IOHandler->enqueue(IOTask(this, aWrite));
}

void
Series::readFileBased( )
{
    Parameter< Operation::OPEN_FILE > fOpen;
    Parameter< Operation::CLOSE_FILE > fClose;
    Parameter< Operation::READ_ATT > aRead;

    if( !auxiliary::directory_exists(IOHandler->directory) )
        throw no_such_file_error("Supplied directory is not valid: " + IOHandler->directory);

    auto isPartOfSeries = matcher(*m_filenamePrefix, *m_filenamePadding, *m_filenamePostfix, *m_format);
    bool isContained;
    int padding;
    std::set< int > paddings;
    for( auto const& entry : auxiliary::list_directory(IOHandler->directory) )
    {
        std::tie(isContained, padding) = isPartOfSeries(entry);
        if( isContained )
        {
            // TODO skip if the padding is exact the number of chars in an iteration?
            paddings.insert(padding);

            fOpen.name = entry;
            IOHandler->enqueue(IOTask(this, fOpen));
            IOHandler->flush();
            iterations.m_writable->parent = getWritable(this);
            iterations.parent = getWritable(this);

            readBase();

            using DT = Datatype;
            aRead.name = "iterationEncoding";
            IOHandler->enqueue( IOTask( this, aRead ) );
            IOHandler->flush();
            if( *aRead.dtype == DT::STRING )
            {
                std::string encoding =
                    Attribute( *aRead.resource ).get< std::string >();
                if( encoding == "fileBased" )
                    *m_iterationEncoding = IterationEncoding::fileBased;
                else if( encoding == "groupBased" )
                {
                    *m_iterationEncoding = IterationEncoding::groupBased;
                    std::cerr << "Series constructor called with iteration "
                                    "regex '%T' suggests loading a "
                                << "time series with fileBased iteration "
                                    "encoding. Loaded file is groupBased.\n";
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
            IOHandler->enqueue( IOTask( this, aRead ) );
            IOHandler->flush();
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

            read();
            IOHandler->enqueue(IOTask(this, fClose));
            IOHandler->flush();
        }
    }

    for( auto & iteration : iterations )
    {
        *iteration.second.m_closed = Iteration::CloseStatus::ClosedTemporarily;
    }

    if( iterations.empty() )
    {
        /* Frontend access type might change during Series::read() to allow parameter modification.
         * Backend access type stays unchanged for the lifetime of a Series. */
        if(IOHandler->m_backendAccess == Access::READ_ONLY  )
            throw std::runtime_error("No matching iterations found: " + name());
        else
            std::cerr << "No matching iterations found: " << name() << std::endl;
    }

    if( paddings.size() == 1u )
        *m_filenamePadding = *paddings.begin();

    /* Frontend access type might change during Series::read() to allow parameter modification.
     * Backend access type stays unchanged for the lifetime of a Series. */
    if( paddings.size() > 1u && IOHandler->m_backendAccess == Access::READ_WRITE )
        throw std::runtime_error("Cannot write to a series with inconsistent iteration padding. "
                                 "Please specify '%0<N>T' or open as read-only.");
}

void
Series::readGroupBased( bool do_init )
{
    Parameter< Operation::OPEN_FILE > fOpen;
    fOpen.name = *m_name;
    IOHandler->enqueue(IOTask(this, fOpen));
    IOHandler->flush();

    if( do_init )
    {
        readBase();

        using DT = Datatype;
        Parameter< Operation::READ_ATT > aRead;
        aRead.name = "iterationEncoding";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();
        if( *aRead.dtype == DT::STRING )
        {
            std::string encoding = Attribute(*aRead.resource).get< std::string >();
            if( encoding == "groupBased" )
                *m_iterationEncoding = IterationEncoding::groupBased;
            else if( encoding == "fileBased" )
            {
                *m_iterationEncoding = IterationEncoding::fileBased;
                std::cerr << "Series constructor called with explicit iteration suggests loading a "
                          << "single file with groupBased iteration encoding. Loaded file is fileBased.\n";
            } else
                throw std::runtime_error("Unknown iterationEncoding: " + encoding);
            setAttribute("iterationEncoding", encoding);
        }
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'iterationEncoding'");

        aRead.name = "iterationFormat";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();
        if( *aRead.dtype == DT::STRING )
        {
            written() = false;
            setIterationFormat(Attribute(*aRead.resource).get< std::string >());
            written() = true;
        }
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'iterationFormat'");
    }

    read();
}

void
Series::readBase()
{
    using DT = Datatype;
    Parameter< Operation::READ_ATT > aRead;

    aRead.name = "openPMD";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    if( *aRead.dtype == DT::STRING )
        setOpenPMD(Attribute(*aRead.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'openPMD'");

    aRead.name = "openPMDextension";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    if( *aRead.dtype == determineDatatype< uint32_t >() )
        setOpenPMDextension(Attribute(*aRead.resource).get< uint32_t >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'openPMDextension'");

    aRead.name = "basePath";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    if( *aRead.dtype == DT::STRING )
        setAttribute("basePath", Attribute(*aRead.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'basePath'");

    Parameter< Operation::LIST_ATTS > aList;
    IOHandler->enqueue(IOTask(this, aList));
    IOHandler->flush();
    if( std::count(aList.attributes->begin(), aList.attributes->end(), "meshesPath") == 1 )
    {
        aRead.name = "meshesPath";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();
        if( *aRead.dtype == DT::STRING )
        {
            /* allow setting the meshes path after completed IO */
            for( auto& it : iterations )
                it.second.meshes.written() = false;

            setMeshesPath(Attribute(*aRead.resource).get< std::string >());

            for( auto& it : iterations )
                it.second.meshes.written() = true;
        }
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'meshesPath'");
    }

    if( std::count(aList.attributes->begin(), aList.attributes->end(), "particlesPath") == 1 )
    {
        aRead.name = "particlesPath";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();
        if( *aRead.dtype == DT::STRING )
        {
            /* allow setting the meshes path after completed IO */
            for( auto& it : iterations )
                it.second.particles.written() = false;

            setParticlesPath(Attribute(*aRead.resource).get< std::string >());


            for( auto& it : iterations )
                it.second.particles.written() = true;
        }
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'particlesPath'");
    }
}

void
Series::read()
{
    Parameter< Operation::OPEN_PATH > pOpen;
    std::string version = openPMD();
    if( version == "1.0.0" || version == "1.0.1" || version == "1.1.0" )
        pOpen.path = auxiliary::replace_first(basePath(), "/%T/", "");
    else
        throw std::runtime_error("Unknown openPMD version - " + version);
    IOHandler->enqueue(IOTask(&iterations, pOpen));

    readAttributes();
    iterations.readAttributes();

    /* obtain all paths inside the basepath (i.e. all iterations) */
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler->enqueue(IOTask(&iterations, pList));
    IOHandler->flush();

    for( auto const& it : *pList.paths )
    {
        Iteration& i = iterations[std::stoull(it)];
        if ( i.closedByWriter( ) )
        {
            continue;
        }
        pOpen.path = it;
        IOHandler->enqueue(IOTask(&i, pOpen));
        i.read();
    }
}

std::string
Series::iterationFilename( uint64_t i )
{
    std::stringstream iteration( "" );
    iteration << std::setw( *m_filenamePadding ) << std::setfill( '0' ) << i;
    return *m_filenamePrefix + iteration.str() + *m_filenamePostfix;
}

Series::iterations_iterator
Series::indexOf( Iteration const & iteration )
{
    for( auto it = iterations.begin(); it != iterations.end(); ++it )
    {
        if( it->second.m_writable.get() == iteration.m_writable.get() )
        {
            return it;
        }
    }
    throw std::runtime_error(
        "[Iteration::close] Iteration not found in Series." );
}

AdvanceStatus
Series::advance(
    AdvanceMode mode,
    Attributable & file,
    iterations_iterator begin,
    Iteration & iteration )
{
    auto end = begin;
    ++end;
    /*
     * @todo By calling flushFileBased/GroupBased, we do not propagate tasks to
     *       the backend yet. We will append ADVANCE and CLOSE_FILE tasks
     *       manually. In order to avoid having them automatically appended by
     *       the flush*Based methods, set CloseStatus to Open for now.
     */
    Iteration::CloseStatus oldCloseStatus = *iteration.m_closed;
    if( oldCloseStatus == Iteration::CloseStatus::ClosedInFrontend )
    {
        *iteration.m_closed = Iteration::CloseStatus::Open;
    }

    switch( *m_iterationEncoding )
    {
        using IE = IterationEncoding;
        case IE::groupBased:
            flushGroupBased( begin, end );
            break;
        case IE::fileBased:
            flushFileBased( begin, end );
            break;
    }
    if( oldCloseStatus == Iteration::CloseStatus::ClosedInFrontend )
    {
        *iteration.m_closed = oldCloseStatus;
    }
    else if(
        oldCloseStatus == Iteration::CloseStatus::ClosedInBackend &&
        *m_iterationEncoding == IterationEncoding::fileBased )
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
        *m_iterationEncoding == IterationEncoding::fileBased )
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
        IOTask task( &file, param );
        IOHandler->enqueue( task );
    }


    if( oldCloseStatus == Iteration::CloseStatus::ClosedInFrontend &&
        mode == AdvanceMode::ENDSTEP )
    {
        using IE = IterationEncoding;
        switch( *m_iterationEncoding )
        {
            case IE::fileBased:
            {
                if( *iteration.m_closed !=
                    Iteration::CloseStatus::ClosedTemporarily )
                {
                    Parameter< Operation::CLOSE_FILE > fClose;
                    IOHandler->enqueue(
                        IOTask( &iteration, std::move( fClose ) ) );
                }
                *iteration.m_closed = Iteration::CloseStatus::ClosedInBackend;
                break;
            }
            case IE::groupBased:
            {
                // We can now put some groups to rest
                Parameter< Operation::CLOSE_PATH > fClose;
                IOHandler->enqueue( IOTask( &iteration, std::move( fClose ) ) );
                // In group-based iteration layout, files are
                // not closed on a per-iteration basis
                // We will treat it as such nonetheless
                *iteration.m_closed = Iteration::CloseStatus::ClosedInBackend;
            }
            break;
        }
    }

    // We cannot call Series::flush now, since the IO handler is still filled
    // from calling flush(Group|File)based, but has not been emptied yet
    // Do that manually
    IOHandler->flush();

    return *param.status;
}

void
Series::openIteration( uint64_t index, Iteration iteration )
{
    // open the iteration's file again
    Parameter< Operation::OPEN_FILE > fOpen;
    fOpen.name = iterationFilename( index );
    IOHandler->enqueue( IOTask( this, fOpen ) );

    /* open base path */
    Parameter< Operation::OPEN_PATH > pOpen;
    pOpen.path = auxiliary::replace_first( basePath(), "%T/", "" );
    IOHandler->enqueue( IOTask( &iterations, pOpen ) );
    /* open iteration path */
    pOpen.path = std::to_string( index );
    IOHandler->enqueue( IOTask( &iteration, pOpen ) );
    switch( *iteration.m_closed )
    {
        using CL = Iteration::CloseStatus;
        case CL::ClosedInBackend:
            throw std::runtime_error(
                "[Series] Detected illegal access to iteration that "
                "has been closed previously." );
        case CL::Open:
        case CL::ClosedTemporarily:
            *iteration.m_closed = CL::Open;
            break;
        case CL::ClosedInFrontend:
            // just keep it like it is
            break;
        default:
            throw std::runtime_error( "Unreachable!" );
    }
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
            case Format::JSON:
                return auxiliary::replace_last(filename, suffix(f), "");
            default:
                return filename;
        }
    }

    std::function<std::tuple<bool, int>(std::string const &)>
    buildMatcher(std::string const &regexPattern) {
        std::regex pattern(regexPattern);

        return [pattern](std::string const &filename) -> std::tuple<bool, int> {
            std::smatch regexMatches;
            bool match = std::regex_match(filename, regexMatches, pattern);
            int padding = match ? regexMatches[1].length() : 0;
            return std::tuple<bool, int>{match, padding};
        };
    }

    std::function<std::tuple<bool, int>(std::string const &)>
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
                return [](std::string const &) -> std::tuple<bool, int> { return std::tuple<bool, int>{false, 0}; };
        }
    }
} // namespace [anonymous]

SeriesIterator::SeriesIterator() : m_series()
{
}

SeriesIterator::SeriesIterator( Series * series ) : m_series( series )
{
    auto it = series->iterations.begin();
    if( it == series->iterations.end() )
    {
        *this = end();
        return;
    }
    else
    {
        auto status = it->second.beginStep();
        if( status == AdvanceStatus::OVER )
        {
            *this = end();
            return;
        }
        it->second.setStepStatus( StepStatus::DuringStep );
    }
    m_currentIteration = it->first;
}

SeriesIterator &
SeriesIterator::operator++()
{
    if( !m_series.has_value() )
    {
        *this = end();
        return *this;
    }
    Series & series = *m_series.get();
    auto & iterations = series.iterations;
    auto & currentIteration = iterations[ m_currentIteration ];
    if( !currentIteration.closed() )
    {
        currentIteration.close();
    }
    switch( *series.m_iterationEncoding )
    {
        using IE = IterationEncoding;
        case IE::groupBased:
        {
            // since we are in group-based iteration layout, it does not
            // matter which iteration we begin a step upon
            AdvanceStatus status = currentIteration.beginStep();
            if( status == AdvanceStatus::OVER )
            {
                *this = end();
                return *this;
            }
            currentIteration.setStepStatus( StepStatus::DuringStep );
            break;
        }
        default:
            break;
    }
    auto it = iterations.find( m_currentIteration );
    auto itEnd = iterations.end();
    if( it == itEnd )
    {
        *this = end();
        return *this;
    }
    ++it;
    if( it == itEnd )
    {
        *this = end();
        return *this;
    }
    m_currentIteration = it->first;
    switch( *series.m_iterationEncoding )
    {
        using IE = IterationEncoding;
        case IE::fileBased:
        {
            auto & iteration = series.iterations[ m_currentIteration ];
            AdvanceStatus status = iteration.beginStep();
            if( status == AdvanceStatus::OVER )
            {
                *this = end();
                return *this;
            }
            iteration.setStepStatus( StepStatus::DuringStep );
            break;
        }
        default:
            break;
    }
    return *this;
}

IndexedIteration
SeriesIterator::operator*()
{
    return IndexedIteration(
        m_series.get()->iterations[ m_currentIteration ], m_currentIteration );
}

bool
SeriesIterator::operator==( SeriesIterator const & other ) const
{
    return this->m_currentIteration == other.m_currentIteration &&
        this->m_series.has_value() == other.m_series.has_value();
}

bool
SeriesIterator::operator!=( SeriesIterator const & other ) const
{
    return !operator==( other );
}

SeriesIterator
SeriesIterator::end()
{
    return {};
}

ReadIterations::ReadIterations( Series * series ) : m_series( series )
{
}

ReadIterations::iterator_t
ReadIterations::begin()
{
    return iterator_t{ m_series };
}

ReadIterations::iterator_t
ReadIterations::end()
{
    return SeriesIterator::end();
}

WriteIterations::SharedResources::SharedResources( iterations_t _iterations )
    : iterations( std::move( _iterations ) )
{
}

WriteIterations::SharedResources::~SharedResources()
{
    if( currentlyOpen.has_value() )
    {
        auto lastIterationIndex = currentlyOpen.get();
        auto & lastIteration = iterations.at( lastIterationIndex );
        if( !lastIteration.closed() )
        {
            lastIteration.close();
        }
    }
}

WriteIterations::WriteIterations( iterations_t iterations )
    : shared{ std::make_shared< SharedResources >( std::move( iterations ) ) }
{
}

WriteIterations::mapped_type &
WriteIterations::operator[]( key_type const & key )
{
    // make a copy
    // explicit cast so MSVC can figure out how to do it correctly
    return operator[]( static_cast< key_type && >( key_type{ key } ) );
}
WriteIterations::mapped_type &
WriteIterations::operator[]( key_type && key )
{
    if( shared->currentlyOpen.has_value() )
    {
        auto lastIterationIndex = shared->currentlyOpen.get();
        auto & lastIteration = shared->iterations.at( lastIterationIndex );
        if( lastIterationIndex != key && !lastIteration.closed() )
        {
            lastIteration.close();
        }
    }
    shared->currentlyOpen = key;
    auto & res = shared->iterations[ std::move( key ) ];
    if( res.getStepStatus() == StepStatus::NoStep )
    {
        res.beginStep();
        res.setStepStatus( StepStatus::DuringStep );
    }
    return res;
}
} // namespace openPMD
