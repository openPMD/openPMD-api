#include <openPMD/binding/c/Series.h>

#include <openPMD/Series.hpp>

#include <string.h>

#include <string>

const openPMD_Attributable *
openPMD_Series_getConstAttributable(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    const auto cxx_attributable = (const openPMD::Attributable *)cxx_series;
    const auto attributable = (const openPMD_Attributable *)cxx_attributable;
    return attributable;
}

openPMD_Attributable *openPMD_Series_getAttributable(openPMD_Series *series)
{
    const auto cxx_series = (openPMD::Series *)series;
    const auto cxx_attributable = (openPMD::Attributable *)cxx_series;
    const auto attributable = (openPMD_Attributable *)cxx_attributable;
    return attributable;
}

openPMD_Series *openPMD_Series_new()
{
    const auto cxx_series = new openPMD::Series();
    const auto series = (openPMD_Series *)cxx_series;
    return series;
}

#if openPMD_HAVE_MPI
openPMD_Series *Series_new_parallell(
    const char *filepath, openPMD_Access at, MPI_Comm comm, const char *options)
{
    const auto cxx_series = new openPMD::Series(
        std::string(filepath),
        openPMD::Access(at),
        comm,
        std::string(options ? options : "{}"));
    const auto series = (openPMD_Series *)cxx_series;
    return series;
}
#endif

openPMD_Series *
Series_new_serial(const char *filepath, openPMD_Access at, const char *options)
{
    const auto cxx_series = new openPMD::Series(
        std::string(filepath),
        openPMD::Access(at),
        std::string(options ? options : "{}"));
    const auto series = (openPMD_Series *)cxx_series;
    return series;
}

void openPMD_Series_delete(openPMD_Series *series)
{
    const auto cxx_series = (openPMD::Series *)series;
    delete cxx_series;
}

bool openPMD_Series_has_value(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return (bool)*cxx_series;
}

char *openPMD_Series_openPMD(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->openPMD().c_str());
}

void openPMD_Series_setOpenPMD(openPMD_Series *series, const char *openPMD)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setOpenPMD(std::string(openPMD));
}

uint32_t openPMD_Series_openPMDextension(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return cxx_series->openPMDextension();
}

void openPMD_Series_setOpenPMDextension(
    openPMD_Series *series, uint32_t openPMDextension)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setOpenPMDextension(openPMDextension);
}

char *openPMD_Series_basePath(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->basePath().c_str());
}

void openPMD_Series_setBasePath(openPMD_Series *series, const char *basePath)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setBasePath(std::string(basePath));
}

char *openPMD_Series_meshesPath(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->meshesPath().c_str());
}

void openPMD_Series_setMeshesPath(
    openPMD_Series *series, const char *meshesPath)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setMeshesPath(std::string(meshesPath));
}

char *openPMD_Series_particlesPath(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->particlesPath().c_str());
}

void openPMD_Series_setParticlesPath(
    openPMD_Series *series, const char *particlesPath)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setParticlesPath(std::string(particlesPath));
}

char *openPMD_Series_author(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->author().c_str());
}

void openPMD_Series_setAuthor(openPMD_Series *series, const char *author)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setAuthor(std::string(author));
}

char *openPMD_Series_software(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->software().c_str());
}

void openPMD_Series_setSoftware(
    openPMD_Series *series, const char *software, const char *version)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setSoftware(std::string(software), std::string(version));
}

char *openPMD_Series_date(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->date().c_str());
}

void openPMD_Series_setDate(openPMD_Series *series, const char *date)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setDate(std::string(date));
}

char *openPMD_Series_softwareDependencies(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->softwareDependencies().c_str());
}

void openPMD_Series_setSoftwareDependencies(
    openPMD_Series *series, const char *softwareDependencies)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setSoftwareDependencies(std::string(softwareDependencies));
}

char *openPMD_Series_machine(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->machine().c_str());
}

void openPMD_Series_setMachine(openPMD_Series *series, const char *machine)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setMachine(std::string(machine));
}

openPMD_IterationEncoding
openPMD_Series_iterationEncoding(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return openPMD_IterationEncoding(cxx_series->iterationEncoding());
}

void openPMD_Series_setIterationEncoding(
    openPMD_Series *series, openPMD_IterationEncoding iterationEncoding)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setIterationEncoding(
        openPMD::IterationEncoding(iterationEncoding));
}

char *openPMD_Series_iterationFormat(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->iterationFormat().c_str());
}

void openPMD_Series_setIterationFOrmat(
    openPMD_Series *series, const char *iterationFormat)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setIterationFormat(std::string(iterationFormat));
}

char *openPMD_Series_name(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->name().c_str());
}

void openPMD_Series_setName(openPMD_Series *series, const char *name)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->setName(std::string(name));
}

char *openPMD_Series_backend(const openPMD_Series *series)
{
    const auto cxx_series = (const openPMD::Series *)series;
    return strdup(cxx_series->backend().c_str());
}

void openPMD_Series_flush(openPMD_Series *series, const char *backendConfig)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->flush(std::string(backendConfig ? backendConfig : "{}"));
}

openPMD_ReadIterations *openPMD_Series_readIteration(openPMD_Series *series)
{
    const auto cxx_series = (openPMD::Series *)series;
    const auto cxx_iterations = cxx_series->readIterations();
    const auto cxx_new_iterations = new openPMD::ReadIterations(cxx_iterations);
    const auto iterations = (openPMD_ReadIterations *)cxx_new_iterations;
    return iterations;
}

void openPMD_Series_parseBase(openPMD_Series *series)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->parseBase();
}

openPMD_WriteIterations *openPMD_Series_writeIteration(openPMD_Series *series)
{
    const auto cxx_series = (openPMD::Series *)series;
    const auto cxx_iterations = cxx_series->writeIterations();
    const auto cxx_new_iterations =
        new openPMD::WriteIterations(cxx_iterations);
    const auto iterations = (openPMD_WriteIterations *)cxx_new_iterations;
    return iterations;
}

void openPMD_Series_close(openPMD_Series *series)
{
    const auto cxx_series = (openPMD::Series *)series;
    cxx_series->close();
}
