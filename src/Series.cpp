/* Copyright 2017 Fabian Koller
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
#include <iostream>
#include <regex>

#include <boost/filesystem.hpp>

#include "auxiliary/StringManip.hpp"
#include "IO/AbstractIOHandler.hpp"
#include "Series.hpp"


void
check_extension(std::string const& filepath)
{
    if( !ends_with(filepath, ".h5") &&
        !ends_with(filepath, ".bp") &&
        !ends_with(filepath, ".dummy") )
        throw std::runtime_error("File format not recognized. "
                                 "Did you append a correct filename extension?");
}

#if openPMD_HAVE_MPI
Series
Series::create(std::string const& filepath,
               MPI_Comm comm,
               AccessType at)
{
    if( AccessType::READ_ONLY == at )
        throw std::runtime_error("Access type not supported in create-API.");

    check_extension(filepath);

    return Series(filepath, at, comm);
}
#endif

Series
Series::create(std::string const& filepath,
               AccessType at)
{
    if( AccessType::READ_ONLY == at )
        throw std::runtime_error("Access type not supported in create-API.");

    check_extension(filepath);

    return Series(filepath, at);
}

#if openPMD_HAVE_MPI
Series
Series::read(std::string const& filepath,
             MPI_Comm comm,
             AccessType at)
{
    if( AccessType::CREATE == at )
        throw std::runtime_error("Access type not supported in read-API.");

    check_extension(filepath);

    return Series(filepath, at, comm);
}
#endif

Series
Series::read(std::string const& filepath,
             AccessType at)
{
    if( AccessType::CREATE == at )
        throw std::runtime_error("Access type not supported in read-API.");

    check_extension(filepath);

    return Series(filepath, at);
}


#if openPMD_HAVE_MPI
Series::Series(std::string const& filepath,
               AccessType at,
               MPI_Comm comm)
        : iterations{Container< Iteration, uint64_t >()}
{
    std::string path;
    std::string name;
    auto const pos = filepath.find_last_of('/');
    if( std::string::npos == pos )
    {
        path = "./";
        name = filepath;
    }
    else
    {
        path = filepath.substr(0, pos + 1);
        name = filepath.substr(pos + 1);
    }

    IterationEncoding ie;
    if( std::string::npos != name.find("%T") )
        ie = IterationEncoding::fileBased;
    else
        ie = IterationEncoding::groupBased;

    Format f;
    if( ends_with(name, ".h5") )
        f = Format::HDF5;
    else if( ends_with(name, ".bp") )
        f = Format::ADIOS1;
    else
    {
        if( !ends_with(name, ".dummy") )
            std::cerr << "Unknown filename extension. "
                         "Falling back to DUMMY format."
                      << std::endl;
        f = Format::DUMMY;
    }

    IOHandler = AbstractIOHandler::createIOHandler(path, at, f, comm);
    iterations.IOHandler = IOHandler;
    iterations.parent = this;

    m_name = cleanFilename(name, f);

    switch( at )
    {
        case AccessType::CREATE:
        {
            setOpenPMD(OPENPMD);
            setOpenPMDextension(0);
            setAttribute("basePath", std::string(BASEPATH));
            if( ie == IterationEncoding::fileBased && !contains(m_name, "%T") )
                throw std::runtime_error("For fileBased formats the iteration regex %T must be included in the file name");
            setIterationEncoding(ie);
            break;
        }
        case AccessType::READ_ONLY:
        case AccessType::READ_WRITE:
        {
            if( contains(m_name, "%T") )
                readFileBased();
            else
                readGroupBased();
            break;
        }
    }
}
#endif

Series::Series(std::string const& filepath,
               AccessType at)
        : iterations{Container< Iteration, uint64_t >()}
{
    std::string path;
    std::string name;
    auto const pos = filepath.find_last_of('/');
    if( std::string::npos == pos )
    {
        path = "./";
        name = filepath;
    }
    else
    {
        path = filepath.substr(0, pos + 1);
        name = filepath.substr(pos + 1);
    }

    IterationEncoding ie;
    if( std::string::npos != name.find("%T") )
        ie = IterationEncoding::fileBased;
    else
        ie = IterationEncoding::groupBased;

    Format f;
    if( ends_with(name, ".h5") )
        f = Format::HDF5;
    else if( ends_with(name, ".bp") )
        f = Format::ADIOS1;
    else
    {
        if( !ends_with(name, ".dummy") )
            std::cerr << "Unknown filename extension. "
                         "Falling back to DUMMY format."
                      << std::endl;
        f = Format::DUMMY;
    }

    IOHandler = AbstractIOHandler::createIOHandler(path, at, f);
    iterations.IOHandler = IOHandler;
    iterations.parent = this;

    m_name = cleanFilename(name, f);

    switch( at )
    {
        case AccessType::CREATE:
        {
            setOpenPMD(OPENPMD);
            setOpenPMDextension(0);
            setAttribute("basePath", std::string(BASEPATH));
            if( ie == IterationEncoding::fileBased && !contains(m_name, "%T") )
                throw std::runtime_error("For fileBased formats the iteration regex %T must be included in the file name");
            setIterationEncoding(ie);
            break;
        }
        case AccessType::READ_ONLY:
        case AccessType::READ_WRITE:
        {
            if( contains(m_name, "%T") )
                readFileBased();
            else
                readGroupBased();
            break;
        }
    }
}

Series::~Series()
{
    flush();
    IOHandler->flush();
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
                    [](Container< Iteration, uint64_t >::value_type const& i){ return i.second.meshes.written; }) )
        throw std::runtime_error("A files meshesPath can not (yet) be changed after it has been written.");

    if( ends_with(mp, "/") )
        setAttribute("meshesPath", mp);
    else
        setAttribute("meshesPath", mp + "/");
    dirty = true;
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
                    [](Container< Iteration, uint64_t >::value_type const& i){ return i.second.particles.written; }) )
        throw std::runtime_error("A files particlesPath can not (yet) be changed after it has been written.");

    if( ends_with(pp, "/") )
        setAttribute("particlesPath", pp);
    else
        setAttribute("particlesPath", pp + "/");
    dirty = true;
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
Series::setSoftware(std::string const& s)
{
    setAttribute("software", s);
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
Series::setSoftwareDependencies(std::string const &softwareDependencies)
{
    setAttribute("softwareDependencies", softwareDependencies);
    return *this;
}

std::string
Series::machine() const
{
    return getAttribute("machine").get< std::string >();
}

Series&
Series::setMachine(std::string const &machine)
{
    setAttribute("machine", machine);
    return *this;
}

IterationEncoding
Series::iterationEncoding() const
{
    return m_iterationEncoding;
}

Series&
Series::setIterationEncoding(IterationEncoding ie)
{
    if( written )
        throw std::runtime_error("A files iterationEncoding can not (yet) be changed after it has been written.");

    m_iterationEncoding = ie;
    switch( ie )
    {
        case IterationEncoding::fileBased:
            setIterationFormat(m_name);
            setAttribute("iterationEncoding", std::string("fileBased"));
            break;
        case IterationEncoding::groupBased:
            setIterationFormat(BASEPATH);
            setAttribute("iterationEncoding", std::string("groupBased"));
            break;
    }
    dirty = true;
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
    if( written )
        throw std::runtime_error("A files iterationFormat can not (yet) be changed after it has been written.");

    if( m_iterationEncoding == IterationEncoding::groupBased )
    {
        if( basePath() != i && (openPMD() == "1.0.1" || openPMD() == "1.0.0") )
            throw std::invalid_argument("iterationFormat must not differ from basePath " + basePath() + " for groupBased data");
    }
    setAttribute("iterationFormat", i);
    return *this;
}

std::string
Series::name() const
{
    return m_name;
}

Series&
Series::setName(std::string const& n)
{
    if( written )
        throw std::runtime_error("A files name can not (yet) be changed after it has been written.");

    if( m_iterationEncoding == IterationEncoding::fileBased && !contains(m_name, "%T") )
            throw std::runtime_error("For fileBased formats the iteration regex %T must be included in the file name");

    m_name = n;
    dirty = true;
    return *this;
}

void
Series::flush()
{
    if( IOHandler->accessType == AccessType::READ_WRITE ||
        IOHandler->accessType == AccessType::CREATE )
    {
        switch( m_iterationEncoding )
        {
            using IE = IterationEncoding;
            case IE::fileBased:
                flushFileBased();
                break;
            case IE::groupBased:
                flushGroupBased();
                break;
        }
    }
}

void
Series::flushFileBased()
{
    if( iterations.empty() )
        throw std::runtime_error("fileBased output can not be written with no iterations.");

    for( auto& i : iterations )
    {
        /* as there is only one series,
         * emulate the file belonging to each iteration as not yet written */
        written = false;
        iterations.written = false;

        i.second.flushFileBased(i.first);

        iterations.flush(replace_first(basePath(), "%T/", ""));

        if( dirty )
        {
            flushAttributes();
            /* manually flag the Series dirty
             * until all iterations have been updated */
            dirty = true;
        }
    }
    dirty = false;
}

void
Series::flushGroupBased()
{
    if( !written )
    {
        Parameter< Operation::CREATE_FILE > fCreate;
        fCreate.name = m_name;
        IOHandler->enqueue(IOTask(this, fCreate));
        IOHandler->flush();
    }

    if( !iterations.written )
        iterations.parent = this;
    iterations.flush(replace_first(basePath(), "%T/", ""));

    for( auto& i : iterations )
    {
        if( !i.second.written )
            i.second.parent = &iterations;
        i.second.flushGroupBased(i.first);
    }

    flushAttributes();
}

void
Series::flushMeshesPath()
{
    //TODO fileBased
    Parameter< Operation::WRITE_ATT > aWrite;
    aWrite.name = "meshesPath";
    Attribute a = getAttribute("meshesPath");
    aWrite.resource = a.getResource();
    aWrite.dtype = a.dtype;
    IOHandler->enqueue(IOTask(this, aWrite));
    IOHandler->flush();
}

void
Series::flushParticlesPath()
{
    //TODO fileBased
    Parameter< Operation::WRITE_ATT > aWrite;
    aWrite.name = "particlesPath";
    Attribute a = getAttribute("particlesPath");
    aWrite.resource = a.getResource();
    aWrite.dtype = a.dtype;
    IOHandler->enqueue(IOTask(this, aWrite));
    IOHandler->flush();
}

void
Series::readFileBased()
{
    std::regex pattern(replace_first(m_name, "%T", "[[:digit:]]+"));

    Parameter< Operation::OPEN_FILE > fOpen;
    Parameter< Operation::READ_ATT > aRead;

    using namespace boost::filesystem;
    path dir = path(IOHandler->directory);
    if( !exists(dir) )
        throw no_such_file_error("Supplied directory is not valid: " + IOHandler->directory);
    for( path const& entry : directory_iterator(dir) )
    {
        if( std::regex_search(entry.filename().string(), pattern) )
        {
            fOpen.name = entry.filename().string();
            IOHandler->enqueue(IOTask(this, fOpen));
            IOHandler->flush();
            iterations.parent = this;

            /* allow all attributes to be set */
            written = false;

            readBase();

            using DT = Datatype;
            aRead.name = "iterationEncoding";
            IOHandler->enqueue(IOTask(this, aRead));
            IOHandler->flush();
            if( *aRead.dtype == DT::STRING )
            {
                std::string encoding = Attribute(*aRead.resource).get< std::string >();
                if( encoding == "fileBased" )
                    m_iterationEncoding = IterationEncoding::fileBased;
                else if( encoding == "groupBased" )
                {
                    m_iterationEncoding = IterationEncoding::groupBased;
                    std::cerr << "Series constructor called with iteration regex '%T' suggests loading a "
                              << "time series with fileBased iteration encoding. Loaded file is groupBased.\n";
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
                setIterationFormat(Attribute(*aRead.resource).get< std::string >());
            else
                throw std::runtime_error("Unexpected Attribute datatype for 'iterationFormat'");

            read();
        }
    }

    /* this file need not be flushed */
    iterations.written = true;
    written = true;
}

void
Series::readGroupBased()
{
    Parameter< Operation::OPEN_FILE > fOpen;
    fOpen.name = m_name;
    IOHandler->enqueue(IOTask(this, fOpen));
    IOHandler->flush();

    /* allow all attributes to be set */
    written = false;

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
            m_iterationEncoding = IterationEncoding::groupBased;
        else if( encoding == "fileBased" )
        {
            m_iterationEncoding = IterationEncoding::fileBased;
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
        setIterationFormat(Attribute(*aRead.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'iterationFormat'");

    /* do not use the public checked version
     * at this point we can guarantee clearing the container won't break anything */
    iterations.clear_unchecked();

    read();

    /* this file need not be flushed */
    iterations.written = true;
    written = true;
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
    if( *aRead.dtype == DT::UINT32 )
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
        /* allow setting the meshes path after completed IO */
        for( auto& it : iterations )
            it.second.meshes.written = false;

        aRead.name = "meshesPath";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();
        if( *aRead.dtype == DT::STRING )
            setMeshesPath(Attribute(*aRead.resource).get< std::string >());
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'meshesPath'");

        for( auto& it : iterations )
            it.second.meshes.written = true;
    }

    if( std::count(aList.attributes->begin(), aList.attributes->end(), "particlesPath") == 1 )
    {
        /* allow setting the meshes path after completed IO */
        for( auto& it : iterations )
            it.second.particles.written = false;

        aRead.name = "particlesPath";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();
        if( *aRead.dtype == DT::STRING )
            setParticlesPath(Attribute(*aRead.resource).get< std::string >());
        else
            throw std::runtime_error("Unexpected Attribute datatype for 'particlesPath'");

        for( auto& it : iterations )
            it.second.particles.written = true;
    }
}

void
Series::read()
{
    Parameter< Operation::OPEN_PATH > pOpen;
    std::string version = openPMD();
    if( version == "1.0.0" || version == "1.0.1" || version == "1.1.0" )
        pOpen.path = replace_first(basePath(), "/%T/", "");
    else
        throw std::runtime_error("Unknown openPMD version - " + version);
    IOHandler->enqueue(IOTask(&iterations, pOpen));
    IOHandler->flush();

    iterations.readAttributes();

    /* obtain all paths inside the basepath (i.e. all iterations) */
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler->enqueue(IOTask(&iterations, pList));
    IOHandler->flush();

    for( auto const& it : *pList.paths )
    {
        Iteration& i = iterations[std::stoull(it)];
        pOpen.path = it;
        IOHandler->enqueue(IOTask(&i, pOpen));
        IOHandler->flush();
        i.read();
    }

    readAttributes();
}

std::string
Series::cleanFilename(std::string s, Format f)
{
    switch( f )
    {
        case Format::HDF5:
            s = replace_last(s, ".h5", "");
            break;
        case Format::ADIOS1:
        case Format::ADIOS2:
            s = replace_last(s, ".bp", "");
            break;
        case Format::DUMMY:
            s = replace_last(s, ".dummy", "");
            break;
        default:
            break;
    }

    return s;
}
