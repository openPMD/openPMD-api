#include "../include/Output.hpp"


std::string const Output::BASEPATH = "/data/%T/";

Output::Output(Output const& o)
        : iterations{o.iterations},
          m_iterationEncoding{o.m_iterationEncoding}
{ }

Output::Output(Output&& o) = default;

Output::Output(IterationEncoding ie)
        : iterations{Container< Iteration, uint64_t >()},
          m_iterationEncoding{ie}
{
    setName("new_openpmd_output");
    setAttribute("basePath", BASEPATH);
    setMeshesPath("meshes");
    setParticlesPath("paticles");
}

Output::Output(IterationEncoding ie, std::string const& name)
        : iterations{Container< Iteration, uint64_t >()},
          m_iterationEncoding{ie}
{
    setName(name);
    setAttribute("basePath", BASEPATH);
    setMeshesPath("meshes");
    setParticlesPath("paticles");
}

Output::Output(IterationEncoding ie,
               std::string const& name,
               std::string const& meshesPath,
               std::string const& particlesPath)
        : iterations{Container< Iteration, uint64_t >()},
          m_iterationEncoding{ie}
{
    setName(name);
    setAttribute("basePath", BASEPATH);
    setMeshesPath(meshesPath);
    setParticlesPath(particlesPath);
}

IterationEncoding
Output::iterationEncoding() const
{
    return m_iterationEncoding;
}

Output
Output::setIterationEncoding(IterationEncoding ie)
{
    m_iterationEncoding = ie;
    return *this;
}

std::string const
Output::name() const
{
    return getAttribute("name");
}

Output
Output::setName(std::string const& n)
{
    setAttribute("name", n);
    return *this;
}

std::string const
Output::basePath() const
{
    return getAttribute("basePath");
}

std::string const
Output::meshesPath() const
{
    return getAttribute("meshesPath");
}

Output
Output::setMeshesPath(std::string const& mp)
{
    setAttribute("meshesPath", mp);
    return *this;
}

std::string const
Output::particlesPath() const
{
    return getAttribute("particlesPath");
}

Output
Output::setParticlesPath(std::string const& pp)
{
    setAttribute("particlesPath", pp);
    return *this;
}
