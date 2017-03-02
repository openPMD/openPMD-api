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

//    Output(Output&&);
//    Output(Output const&);
    Output(IterationEncoding iterationEncoding);
    Output(IterationEncoding iterationEncoding,
           std::string const& name);
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
    Output& setIterationEncoding(IterationEncoding iterationEncoding);

    std::string const name() const;
    Output& setName(std::string const& name);

    std::string const basePath() const;
//    Output& setBasePath(std::string const &); //Custom basePath not available in openPMD <=1.0.1

    std::string const meshesPath() const;
    Output& setMeshesPath(std::string const&);

    std::string const particlesPath() const;
    Output& setParticlesPath(std::string const&);

//    void write();

    Container< Iteration, uint64_t > iterations;

    static std::string const BASEPATH;

private:
    IterationEncoding m_iterationEncoding;
};  //Output

std::ostream&
operator<<(std::ostream& os, Output::IterationEncoding ie);
