#include <iostream>
#include <set>

#include "../include/Auxiliary.hpp"
#include "../include/Output.hpp"
#include "../include/IO/HDF5/HDF5IOHandler.hpp"


char const * const Output::BASEPATH = "/data/%T/";
char const * const Output::OPENPMD = "1.0.1";

std::ostream&
operator<<(std::ostream& os, Output::IterationEncoding ie)
{
    switch( ie )
    {
        case Output::IterationEncoding::fileBased:
            os<<"fileBased";
            break;
        case Output::IterationEncoding::groupBased:
            os<<"groupBased";
            break;
    }
    return os;
}

Output::Output(std::string const& path,
               std::string const& name,
               IterationEncoding ie,
               Format f,
               AccessType at)
        : iterations{Container< Iteration, uint64_t >()},
          m_iterationEncoding{ie},
          m_name{name}
{
    switch( f )
    {
        case Format::HDF5:
            IOHandler = std::make_shared<HDF5IOHandler>(path, at);
            break;
        case Format::ADIOS:
            break;
        case Format::NONE:
            IOHandler = std::make_shared<NONEIOHandler>(path, at);
            break;
    }
    iterations.IOHandler = IOHandler;
    iterations.parent = this;
    switch( at )
    {
        case AccessType::CREAT:
        {
            setOpenPMD(OPENPMD);
            setOpenPMDextension(0);
            setAttribute("basePath", BASEPATH);
            setMeshesPath("meshes/");
            setParticlesPath("particles/");
            switch( ie )
            {
                case Output::IterationEncoding::fileBased:
                    setIterationFormat(m_name + "_%T");
                    setAttribute("iterationEncoding", "fileBased");
                    break;
                case Output::IterationEncoding::groupBased:
                    setIterationFormat("/data/%T/");
                    setAttribute("iterationEncoding", "groupBased");
                    break;
            }
            break;
        }
        case AccessType::READ_ONLY:
        {
            read();
            break;
        }
        case AccessType::READ_WRITE:
        {
            read();
            break;
        }
    }
}

std::string
Output::openPMD() const
{
    return getAttribute("openPMD").get< std::string >();
}

Output&
Output::setOpenPMD(std::string const& o)
{
    setAttribute("openPMD", o);
    dirty = true;
    return *this;
}

uint32_t
Output::openPMDextension() const
{
    return getAttribute("openPMDextension").get< uint32_t >();
}

Output&
Output::setOpenPMDextension(uint32_t oe)
{
    setAttribute("openPMDextension", oe);
    dirty = true;
    return *this;
}

std::string
Output::basePath() const
{
    return getAttribute("basePath").get< std::string >();
}

std::string
Output::meshesPath() const
{
    return getAttribute("meshesPath").get< std::string >();
}

Output&
Output::setMeshesPath(std::string const& mp)
{
    if( ends_with(mp, "/") )
        setAttribute("meshesPath", mp);
    else
        setAttribute("meshesPath", mp + "/");
    dirty = true;
    return *this;
}

std::string
Output::particlesPath() const
{
    return getAttribute("particlesPath").get< std::string >();
}

Output&
Output::setParticlesPath(std::string const& pp)
{
    if( ends_with(pp, "/") )
        setAttribute("particlesPath", pp);
    else
        setAttribute("particlesPath", pp + "/");
    dirty = true;
    return *this;
}

Output::IterationEncoding
Output::iterationEncoding() const
{
    return m_iterationEncoding;
}

std::string
Output::iterationFormat() const
{
    return getAttribute("iterationFormat").get< std::string >();
}

Output&
Output::setIterationFormat(std::string const& i)
{
    if( m_iterationEncoding == IterationEncoding::groupBased )
    {
        if( basePath() != i && (openPMD() == "1.0.1" || openPMD() == "1.0.0") )
            throw std::invalid_argument("iterationFormat must not differ from basePath " + basePath());
    }
    if( m_iterationEncoding == IterationEncoding::fileBased )
    {
        if( i.find("/") != std::string::npos )
            throw std::invalid_argument("iterationFormat must not contain slashes");
    }
    setAttribute("iterationFormat", i);
    dirty = true;
    return *this;
}

std::string
Output::name() const
{
    return m_name;
}

Output&
Output::setName(std::string const& n)
{
    if( !written )
    {
        m_name = n;
        dirty = true;
    } else
        throw std::runtime_error("A files name can not (yet) be changed after it has been written.");
    return *this;
}

void
Output::flush()
{
    switch( m_iterationEncoding )
    {
        using IE = IterationEncoding;
        case IE::fileBased:
        {
            for( auto& i : iterations )
            {
                if( !i.second.parent )
                    i.second.parent = this;
                i.second.flushFileBased(i.first);

                // TODO same problem as below
                // [HDF5 backend: Container only corresponds to the ID written the earliest]
                iterations.flush(replace_first(basePath(), "%T/", ""));

                if( dirty )
                {
                    Parameter< Operation::WRITE_ATT > attribute_parameter;
                    for( std::string const & att_name : attributes() )
                    {
                        attribute_parameter.name = att_name;
                        attribute_parameter.resource = getAttribute(att_name).getResource();
                        attribute_parameter.dtype = getAttribute(att_name).dtype;
                        // TODO "this" is too general for writing file-based root attributes
                        // [HDF5 backend: only one fileID is saved per Writable,
                        //  thus this Output only corresponds to the ID written the earliest]
                        IOHandler->enqueue(IOTask(this, attribute_parameter));
                    }
                    IOHandler->flush();
                }
            }
            break;
        }
        case IE::groupBased:
        {
            if( !written )
            {
                Parameter< Operation::CREATE_FILE > file_parameter;
                file_parameter.name = m_name;
                IOHandler->enqueue(IOTask(this, file_parameter));
                IOHandler->flush();
            }

            if( !iterations.parent )
                iterations.parent = this;
            iterations.flush(replace_first(basePath(), "%T/", ""));

            for( auto& i : iterations )
            {
                if( !i.second.parent )
                    i.second.parent = &iterations;
                i.second.flushGroupBased(i.first);
            }

            if( dirty )
            {
                Parameter< Operation::WRITE_ATT > attribute_parameter;
                for( std::string const & att_name : attributes() )
                {
                    attribute_parameter.name = att_name;
                    attribute_parameter.resource = getAttribute(att_name).getResource();
                    attribute_parameter.dtype = getAttribute(att_name).dtype;
                    IOHandler->enqueue(IOTask(this, attribute_parameter));
                }
            }

        }
        break;
    }

    IOHandler->flush();
    dirty = false;
}

void
Output::read()
{
    Parameter< Operation::OPEN_FILE > file_parameter;
    file_parameter.name = m_name;
    IOHandler->enqueue(IOTask(this, file_parameter));
    IOHandler->flush();

    Parameter< Operation::LIST_ATTS > list_parameter;
    IOHandler->enqueue(IOTask(this, list_parameter));
    IOHandler->flush();
    std::set< std::string > attributes(list_parameter.attributes->begin(),
                                       list_parameter.attributes->end());

    using DT = Datatype;
    Parameter< Operation::READ_ATT > attribute_parameter;

    attribute_parameter.name = "openPMD";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::STRING )
        setOpenPMD(Attribute(*attribute_parameter.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'openPMD'");
    attributes.erase("openPMD");

    attribute_parameter.name = "openPMDextension";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::UINT32 )
        setOpenPMDextension(Attribute(*attribute_parameter.resource).get< uint32_t >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'openPMDextension'");
    attributes.erase("openPMDextension");

    attribute_parameter.name = "basePath";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::STRING )
        setAttribute("basePath", Attribute(*attribute_parameter.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'basePath'");
    attributes.erase("basePath");

    attribute_parameter.name = "meshesPath";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::STRING )
        setMeshesPath(Attribute(*attribute_parameter.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'meshesPath'");
    attributes.erase("meshesPath");

    attribute_parameter.name = "particlesPath";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::STRING )
        setParticlesPath(Attribute(*attribute_parameter.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'particlesPath'");
    attributes.erase("particlesPath");

    attribute_parameter.name = "iterationEncoding";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::STRING )
    {
        std::string encoding = Attribute(*attribute_parameter.resource).get< std::string >();
        if( encoding == "fileBased" )
        {
            if( m_iterationEncoding != IterationEncoding::fileBased)
                throw std::runtime_error("Supplied and read iterationEncoding do not match");
        }
        else if( encoding == "groupBased" )
        {
            if( m_iterationEncoding != IterationEncoding::groupBased)
                throw std::runtime_error("Supplied and read iterationEncoding do not match");
        } else
            throw std::runtime_error("Unknown iterationEncoding: " + encoding);
        setAttribute("iterationEncoding", encoding);
    }
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'iterationEncoding'");
    attributes.erase("iterationEncoding");

    attribute_parameter.name = "iterationFormat";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::STRING )
        setIterationFormat(Attribute(*attribute_parameter.resource).get< std::string >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'iterationFormat'");
    attributes.erase("iterationFormat");

    for( auto const& att : attributes )
    {
        attribute_parameter.name = "iterationFormat";
        IOHandler->enqueue(IOTask(this, attribute_parameter));
        IOHandler->flush();
        Attribute a(*attribute_parameter.resource);
        switch( *attribute_parameter.dtype )
        {
            case DT::CHAR:
                setAttribute(att, a.get< char >());
                break;
            case DT::INT:
                setAttribute(att, a.get< int >());
                break;
            case DT::FLOAT:
                setAttribute(att, a.get< float >());
                break;
            case DT::DOUBLE:
                setAttribute(att, a.get< double >());
                break;
            case DT::UINT32:
                setAttribute(att, a.get< uint32_t >());
                break;
            case DT::UINT64:
                setAttribute(att, a.get< uint64_t >());
                break;
            case DT::STRING:
                setAttribute(att, a.get< std::string >());
                break;
            case DT::ARR_DBL_7:
                setAttribute(att, a.get< std::array< double, 7 > >());
                break;
            case DT::VEC_INT:
                setAttribute(att, a.get< std::vector< int > >());
                break;
            case DT::VEC_FLOAT:
                setAttribute(att, a.get< std::vector< float > >());
                break;
            case DT::VEC_DOUBLE:
                setAttribute(att, a.get< std::vector< double > >());
                break;
            case DT::VEC_UINT64:
                setAttribute(att, a.get< std::vector< uint64_t > >());
                break;
            case DT::VEC_STRING:
                setAttribute(att, a.get< std::vector< std::string > >());
                break;
        }
    }

    //TODO find solution for handling fileBased output
    //TODO recursively read Iterations
}
