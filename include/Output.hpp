#pragma once


#include "Attributable.hpp"
#include "Container.hpp"
#include "IO/AbstractIOHandler.hpp"
#include "IO/AccessType.hpp"
#include "IO/Format.hpp"
#include "Iteration.hpp"
#include "Writable.hpp"


class Output : public Attributable
{
public:
    enum class IterationEncoding
    {
        fileBased, groupBased
    };  //IterationEncoding

    Output(std::string const& path,
           std::string const& name,
           IterationEncoding ie,
           Format f,
           AccessType at);

    std::string openPMD() const;
    Output& setOpenPMD(std::string const&);

    uint32_t openPMDextension() const;
    Output& setOpenPMDextension(uint32_t);

    std::string basePath() const;
//    Output& setBasePath(std::string const &); //Custom basePath not available in openPMD <=1.0.1

    std::string meshesPath() const;
    Output& setMeshesPath(std::string const&);

    std::string particlesPath() const;
    Output& setParticlesPath(std::string const&);

    IterationEncoding iterationEncoding() const;
//    Output& setIterationEncoding(IterationEncoding); //Allowing this makes writing extremely messy

    std::string iterationFormat() const;
    Output& setIterationFormat(std::string const&);

    std::string name() const;
    Output& setName(std::string const&);

    void flush();

    Container< Iteration, uint64_t > iterations;

private:
    void read();

    static char const * const BASEPATH;
    static char const * const OPENPMD;
    IterationEncoding m_iterationEncoding;
    std::string m_name;
};  //Output

std::ostream&
operator<<(std::ostream& os, Output::IterationEncoding ie);
