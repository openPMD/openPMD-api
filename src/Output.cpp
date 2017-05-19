#include <iostream>

#include "../include/Auxiliary.hpp"
#include "../include/Output.hpp"
#include "../include/IO/HDF5/HDF5IOHandler.hpp"


std::string const Output::BASEPATH = "/data/%T/";
std::string const Output::OPENPMD = "1.0.1";

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
            IOHandler = std::make_unique<HDF5IOHandler>(path, at);
            break;
        case Format::ADIOS:
            //TODO
            break;
        case Format::NONE:
            IOHandler = std::make_unique<NONEIOHandler>(path, at);
    }
    iterations.IOHandler = IOHandler;
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
            //TODO write file
            setIterationFormat("/data/%T/");
            setAttribute("iterationEncoding", "groupBased");
            break;
    }
}

std::string
Output::openPMD() const
{
    return boost::get< std::string >(getAttribute("openPMD").getResource());
}

Output&
Output::setOpenPMD(std::string const& o)
{
    setAttribute("openPMD", o);
    return *this;
}

uint32_t
Output::openPMDextension() const
{
    return boost::get< uint32_t>(getAttribute("openPMDextension").getResource());
}

Output&
Output::setOpenPMDextension(uint32_t oe)
{
    setAttribute("openPMDextension", oe);
    return *this;
}

std::string
Output::basePath() const
{
    return boost::get< std::string >(getAttribute("basePath").getResource());
}

std::string
Output::meshesPath() const
{
    return boost::get< std::string >(getAttribute("meshesPath").getResource());
}

Output&
Output::setMeshesPath(std::string const& mp)
{
    if( ends_with(mp, "/") )
        setAttribute("meshesPath", mp);
    else
        setAttribute("meshesPath", mp + "/");
    return *this;
}

std::string
Output::particlesPath() const
{
    return boost::get< std::string >(getAttribute("particlesPath").getResource());
}

Output&
Output::setParticlesPath(std::string const& pp)
{
    if( ends_with(pp, "/") )
        setAttribute("particlesPath", pp);
    else
        setAttribute("particlesPath", pp + "/");
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
    return boost::get< std::string >(getAttribute("iterationFormat").getResource());
}

Output&
Output::setIterationFormat(std::string const& i)
{
    if( m_iterationEncoding == IterationEncoding::groupBased )
    {
        if( basePath() != i && (openPMD() == "1.0.1" || openPMD() == "1.0.0") )
            throw std::invalid_argument( "iterationFormat must not differ from basePath " + basePath());
    }
    if( m_iterationEncoding == IterationEncoding::fileBased )
    {
        if( i.find("/") != std::string::npos )
            throw std::invalid_argument( "iterationFormat must not contain slashes");
    }
    setAttribute("iterationFormat", i);
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
    m_name = n;
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
            if( !written )
            {
                Parameter< Operation::CREATE_FILE > file_parameter;
                file_parameter.name = m_name;
                file_parameter.basePath = getAttribute("basePath").get< std::string >();
                IOHandler->enqueue(IOTask(this, file_parameter));
            }
            Parameter< Operation::WRITE_ATT > attribute_parameter;
            attribute_parameter.name = "iterationFormat";
            attribute_parameter.resource = Attribute(std::string("fileBased")).getResource();
            attribute_parameter.dtype = Attribute::Dtype::STRING;
            IOHandler->enqueue(IOTask(this, attribute_parameter));

            for( std::string const & att_name : attributes() )
            {
                attribute_parameter.name = att_name;
                attribute_parameter.resource = getAttribute(att_name).getResource();
                attribute_parameter.dtype = getAttribute(att_name).dtype;
                IOHandler->enqueue(IOTask(this, attribute_parameter));
            }
        }
            break;
        case IE::groupBased:
        {

        }
        break;
    }

    IOHandler->flush();
}
