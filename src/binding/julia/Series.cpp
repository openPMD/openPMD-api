// Series

#include "defs.hpp"

#include <cstring>

#if openPMD_HAVE_MPI
#include <mpi.h>

namespace jlcxx
{
template <>
struct IsMirroredType<MPI_Comm> : std::false_type
{};
} // namespace jlcxx
#endif

// Define supertype relationships
namespace jlcxx
{
template <>
struct SuperType<Series>
{
    using type = Attributable;
};
} // namespace jlcxx

void define_julia_Series(jlcxx::Module &mod)
{

    // Series

    auto type = mod.add_type<Series>(
        "CXX_Series", jlcxx::julia_base_type<Attributable>());

    type.constructor<>();
#if openPMD_HAVE_MPI
    type.method(
        "cxx_Series",
        [](const std::string &filepath,
           Access at,
           sized_uint_t<sizeof(MPI_Comm)> ucomm,
           const std::string &options) {
            MPI_Comm comm;
            static_assert(sizeof ucomm == sizeof comm);
            memcpy(&comm, &ucomm, sizeof comm);
            return Series(filepath, at, comm, options);
        });
    type.method(
        "cxx_Series",
        [](const std::string &filepath,
           Access at,
           sized_uint_t<sizeof(MPI_Comm)> ucomm) {
            MPI_Comm comm;
            static_assert(sizeof ucomm == sizeof comm);
            memcpy(&comm, &ucomm, sizeof comm);
            return Series(filepath, at, comm);
        });
#endif
    type.constructor<const std::string &, Access, const std::string &>();
    type.constructor<const std::string &, Access>();

    type.method(
        "cxx_isvalid", [](const Series &series) { return bool(series); });

    type.method("cxx_openPMD_version", &Series::openPMD);
    type.method("cxx_set_openPMD_version!", &Series::setOpenPMD);
    type.method("cxx_openPMD_extension", &Series::openPMDextension);
    type.method("cxx_set_openPMD_extension!", &Series::setOpenPMDextension);
    type.method("cxx_base_path", &Series::basePath);
    type.method("cxx_set_base_path!", &Series::setBasePath);
    type.method("cxx_meshes_path", &Series::meshesPath);
    type.method("cxx_set_meshes_path!", &Series::setMeshesPath);
    type.method("cxx_particles_path", &Series::particlesPath);
    type.method("cxx_set_particles_path!", &Series::setParticlesPath);
    type.method("cxx_author", &Series::author);
    type.method("cxx_set_author!", &Series::setAuthor);
    type.method("cxx_software", &Series::software);
    type.method(
        "cxx_set_software!",
        overload_cast<const std::string &, const std::string &>(
            &Series::setSoftware));
    type.method(
        "cxx_set_software!",
        [](Series &series, const std::string &newName) -> Series & {
            return series.setSoftware(newName);
        });
    type.method("cxx_software_version", &Series::softwareVersion);
    // type.method("set_software_version!",
    // &Series::setSoftwareVersion);
    type.method("cxx_date", &Series::date);
    type.method("cxx_set_date!", &Series::setDate);
    type.method("cxx_software_dependencies", &Series::softwareDependencies);
    type.method(
        "cxx_set_software_dependencies!", &Series::setSoftwareDependencies);
    type.method("cxx_machine", &Series::machine);
    type.method("cxx_set_machine!", &Series::setMachine);
    // TODO: type.method("iteration_encoding",
    // &Series::iterationEncoding);
    // TODO: type.method("set_iteration_encoding!",
    // &Series::setIterationEncoding);
    type.method("cxx_iteration_format", &Series::iterationFormat);
    type.method("cxx_set_iteration_format!", &Series::setIterationFormat);
    type.method("cxx_name", &Series::name);
    type.method("cxx_set_name!", &Series::setName);
    type.method("cxx_backend", &Series::backend);
    type.method("cxx_flush", &Series::flush);

    type.method(
        "cxx_iterations",
        [](Series &series) -> Container<Iteration, uint64_t> & {
            return series.iterations;
        });
    // TODO type.method("read_iterations", &Series::readIterations);
    type.method("cxx_write_iterations", &Series::writeIterations);
}
