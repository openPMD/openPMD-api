#pragma once


#include "Attributable.tpp"
#include "Container.hpp"
#include "Iteration.h"


class Output : public Attributable
{
public:
    Output(Output const&);
    Output(Output&&);
    Output(IterationEncoding iterationEncoding);
    Output(IterationEncoding iterationEncoding, std::string const& name);
    Output(IterationEncoding iterationEncoding,
           std::string const& name,
           std::string const& meshesPath,
           std::string const& particlesPath);
//    Output(IterationEncoding,
//           std::string const &name,
//           std::string const &basePath,
//           std::string const &meshesPath,
//           std::string const &particlesPath);

    IterationEncoding iterationEncoding() const;
    Output setIterationEncoding(IterationEncoding iterationEncoding);

    std::string const name() const;
    Output setName(std::string const& name);

    std::string const basePath() const;
//    Output setBasePath(std::string const &); //Custom basePath not available in openPMD <=1.0.1

    std::string const meshesPath() const;
    Output setMeshesPath(std::string const&);

    std::string const particlesPath() const;
    Output setParticlesPath(std::string const&);

//    void write();

    Container< Iteration, uint64_t > iterations;

    static std::string const BASEPATH;

private:
    IterationEncoding m_iterationEncoding;
};  //Output
