#ifndef OPENPMD_SERIES_H
#define OPENPMD_SERIES_H

#include <openPMD/config.hpp>

#include <openPMD/binding/c/Container_Iteration.h>
#include <openPMD/binding/c/IO/Access.h>
#include <openPMD/binding/c/IterationEncoding.h>
#include <openPMD/binding/c/ReadIterations.h>
#include <openPMD/binding/c/WriteIterations.h>
#include <openPMD/binding/c/backend/Attributable.h>

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Series openPMD_Series;

    // returns a non-owning pointer
    const openPMD_Attributable *
    openPMD_Series_getConstAttributable(const openPMD_Series *series);

    // returns a non-owning pointer
    openPMD_Attributable *
    openPMD_Series_getAttributable(openPMD_Series *series);

    openPMD_Series *openPMD_Series_new();

#if openPMD_HAVE_MPI
    openPMD_Series *openPMD_Series_new_parallel(
        const char *filepath,
        openPMD_Access at,
        MPI_Comm comm,
        const char *options);
#endif

    openPMD_Series *openPMD_Series_new_serial(
        const char *filepath, openPMD_Access at, const char *options);

    void openPMD_Series_delete(openPMD_Series *series);

    // returns a non-owning pointer
    const openPMD_Container_Iteration *
    openPMD_Series_constIterations(const openPMD_Series *series);

    // returns a non-owning pointer
    openPMD_Container_Iteration *
    openPMD_Series_iterations(openPMD_Series *series);

    bool openPMD_Series_has_value(const openPMD_Series *series);

    char *openPMD_Series_openPMD(const openPMD_Series *series);

    void openPMD_Series_setOpenPMD(openPMD_Series *series, const char *openPMD);

    uint32_t openPMD_Series_openPMDextension(const openPMD_Series *series);

    void openPMD_Series_setOpenPMDextension(
        openPMD_Series *series, uint32_t openPMDextension);

    char *openPMD_Series_basePath(const openPMD_Series *series);

    void
    openPMD_Series_setBasePath(openPMD_Series *series, const char *basePath);

    char *openPMD_Series_meshesPath(const openPMD_Series *series);

    void openPMD_Series_setMeshesPath(
        openPMD_Series *series, const char *meshesPath);

    char *openPMD_Series_particlesPath(const openPMD_Series *series);

    void openPMD_Series_setParticlesPath(
        openPMD_Series *series, const char *particlesPath);

    char *openPMD_Series_author(const openPMD_Series *series);

    void openPMD_Series_setAuthor(openPMD_Series *series, const char *author);

    char *openPMD_Series_software(const openPMD_Series *series);

    void openPMD_Series_setSoftware(
        openPMD_Series *series, const char *name, const char *version);

    char *openPMD_Series_date(const openPMD_Series *series);

    void openPMD_Series_setDate(openPMD_Series *series, const char *date);

    char *openPMD_Series_softwareDependencies(const openPMD_Series *series);

    void openPMD_Series_setSoftwareDependencies(
        openPMD_Series *series, const char *SoftwareDependencies);

    char *openPMD_Series_machine(const openPMD_Series *series);

    void openPMD_Series_setMachine(openPMD_Series *series, const char *machine);

    openPMD_IterationEncoding
    openPMD_Series_iterationEncoding(const openPMD_Series *series);

    void openPMD_Series_setIterationEncoding(
        openPMD_Series *series, openPMD_IterationEncoding iterationEncoding);

    char *openPMD_Series_iterationFormat(const openPMD_Series *series);

    void openPMD_Series_setIterationFormat(
        openPMD_Series *series, const char *iterationFormat);

    char *openPMD_Series_name(const openPMD_Series *series);

    void openPMD_Series_setName(openPMD_Series *series, const char *name);

    char *openPMD_Series_backend(const openPMD_Series *series);

    void
    openPMD_Series_flush(openPMD_Series *series, const char *backendConfig);

    openPMD_ReadIterations *
    openPMD_Series_readIteration(openPMD_Series *series);

    void openPMD_Series_parseBase(openPMD_Series *series);

    openPMD_WriteIterations *
    openPMD_Series_writeIteration(openPMD_Series *series);

    void openPMD_Series_close(openPMD_Series *series);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_SERIES_H
