#pragma once


#include <iosfwd>

#include "Attributable.hpp"
#include "Container.hpp"
#include "Iteration.hpp"


class Output : public Attributable
{
public:
    enum class IterationEncoding
    {
        fileBased, groupBased
    };  //IterationEncoding

    Output(IterationEncoding iterationEncoding);
    Output(IterationEncoding iterationEncoding,
           std::string const& name);
    Output(IterationEncoding iterationEncoding,
           std::string const& name,
           std::string const& meshesPath,
           std::string const& particlesPath);

    std::string openPMD() const;
    Output& setOpenPMD(std::string const &);

    uint32_t openPMDextension() const;
    Output& setOpenPMDextension(uint32_t);

    std::string basePath() const;
//    Output& setBasePath(std::string const &); //Custom basePath not available in openPMD <=1.0.1

    std::string meshesPath() const;
    Output& setMeshesPath(std::string const&);

    std::string particlesPath() const;
    Output& setParticlesPath(std::string const&);

    IterationEncoding iterationEncoding() const;
    Output& setIterationEncoding(IterationEncoding);

    std::string iterationFormat() const;
    Output& setIterationFormat(std::string const&);

    std::string name() const;
    Output& setName(std::string const&);

    Container< Iteration, uint64_t > iterations;

private:
    void setIterationAttributes(IterationEncoding);
    static std::string const BASEPATH;
    static std::string const OPENPMD;
    IterationEncoding m_iterationEncoding;
    std::string m_name;
};  //Output

std::ostream&
operator<<(std::ostream& os, Output::IterationEncoding ie);
