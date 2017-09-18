#pragma once


#include "Attributable.hpp"
#include "Container.hpp"
#include "IO/AbstractIOHandler.hpp"
#include "IO/AccessType.hpp"
#include "IO/Format.hpp"
#include "Iteration.hpp"
#include "Writable.hpp"


enum class IterationEncoding
{
    fileBased, groupBased
};  //IterationEncoding

/**
 *
 */
class Output : public Attributable
{
public:

    Output(std::string const& path,
           std::string const& name,
           IterationEncoding,
           Format,
           AccessType);
    Output(std::string const& path,
           std::string const& name,
           bool parallel = false);
    ~Output();

    std::string openPMD() const;
    Output& setOpenPMD(std::string const&);

    uint32_t openPMDextension() const;
    Output& setOpenPMDextension(uint32_t);

    std::string basePath() const;
    Output& setBasePath(std::string const &);

    std::string meshesPath() const;
    Output& setMeshesPath(std::string const&);

    std::string particlesPath() const;
    Output& setParticlesPath(std::string const&);

    std::string author() const;
    Output& setAuthor(std::string const&);

    std::string software() const;
    Output& setSoftware(std::string const&);

    std::string softwareVersion() const;
    Output& setSoftwareVersion(std::string const&);

    std::string date() const;
    Output& setDate(std::string const&);

    IterationEncoding iterationEncoding() const;
    Output& setIterationEncoding(IterationEncoding);

    std::string iterationFormat() const;
    Output& setIterationFormat(std::string const&);

    std::string name() const;
    Output& setName(std::string const&);

    void flush();

    Container< Iteration, uint64_t > iterations;

private:
    void flushFileBased();
    void flushGroupBased();
    void read();

    static char const * const BASEPATH;
    static char const * const OPENPMD;
    IterationEncoding m_iterationEncoding;
    std::string m_name;
};  //Output

std::ostream&
operator<<(std::ostream&, IterationEncoding);
