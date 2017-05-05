#include <iostream>

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
    setAttribute("meshesPath", mp);
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
    setAttribute("particlesPath", pp);
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
    //std::map< std::string, Attribute > parameter;
    switch( m_iterationEncoding )
    {
        using IE = IterationEncoding;
        case IE::fileBased:
        {
            Parameter< Operation::CREATE_FILE > file_parameter;
            file_parameter.openPMD = getAttribute("openPMD").get< std::string >();
            file_parameter.openPMDextension = getAttribute("openPMDextension").get< uint32_t >();
            file_parameter.basePath = getAttribute("basePath").get< std::string >();
            file_parameter.meshesPath = getAttribute("meshesPath").get< std::string >();
            file_parameter.particlesPath = getAttribute("particlesPath").get< std::string >();
            file_parameter.iterationFormat = getAttribute("iterationFormat").get< std::string >();
            file_parameter.name = m_name;

            for( auto i : iterations )
            {
                file_parameter.iteration = i.first;
                IOHandler->enqueue(
                        IOTask(std::shared_ptr< Writable >(this),
                               Operation::CREATE_FILE,
                               file_parameter)
                );
                for( auto const & name : attributes() )
                {
                    Parameter< Operation::WRITE_ATT > att_parameter;
                    att_parameter.name = name;
                    Attribute a = getAttribute(name);
                    att_parameter.resource = a.getResource();
                    att_parameter.dtype = a.dtype;
                    IOHandler->enqueue(IOTask(std::shared_ptr< Writable >(this),
                                              Operation::WRITE_ATT,
                                              att_parameter));
                }
            }
            break;
        }
        case IE::groupBased:
            //TODO
            break;
    }

    IOHandler->flush();
}
