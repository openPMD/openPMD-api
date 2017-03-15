#include <iostream>

#include "../include/Output.hpp"


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

Output::Output(IterationEncoding ie)
        : iterations{Container< Iteration, uint64_t >()},
          m_name{"new_openpmd_output"}
{
    setOpenPMD(OPENPMD);
    setOpenPMDextension(0);
    setAttribute("basePath", BASEPATH);
    setMeshesPath("meshes/");
    setParticlesPath("particles/");
    setIterationAttributes(ie);
}

Output::Output(IterationEncoding ie, std::string const& name)
        : iterations{Container< Iteration, uint64_t >()},
          m_name{name}
{
    setOpenPMD(OPENPMD);
    setOpenPMDextension(0);
    setAttribute("basePath", BASEPATH);
    setMeshesPath("meshes/");
    setParticlesPath("particles/");
    setIterationAttributes(ie);
}

Output::Output(IterationEncoding ie,
               std::string const& name,
               std::string const& meshesPath,
               std::string const& particlesPath)
        : iterations{Container< Iteration, uint64_t >()},
          m_name{name}
{
    setOpenPMD(OPENPMD);
    setOpenPMDextension(0);
    setAttribute("basePath", BASEPATH);
    setMeshesPath(meshesPath);
    setParticlesPath(particlesPath);
    setIterationAttributes(ie);
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

Output&
Output::setIterationEncoding(Output::IterationEncoding ie)
{
    setIterationAttributes(ie);
    return *this;
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
Output::setIterationAttributes(IterationEncoding ie)
{
    m_iterationEncoding = ie;
    switch( ie )
    {
        case Output::IterationEncoding::fileBased:
            setAttribute("iterationEncoding", "fileBased");
            setIterationFormat(m_name + "_%T");
            break;
        case Output::IterationEncoding::groupBased:
            setAttribute("iterationEncoding", "groupBased");
            setIterationFormat("/data/%T/");
            break;
    }
}
