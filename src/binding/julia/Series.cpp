// Series

#include "defs.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>

namespace jlcxx {
template <> struct IsMirroredType<MPI_Comm> : std::false_type {};
} // namespace jlcxx
#endif

// Define supertype relationships
namespace jlcxx {
template <> struct SuperType<SeriesInterface> { typedef Attributable type; };
template <> struct SuperType<Series> { typedef SeriesInterface type; };
} // namespace jlcxx

void define_julia_Series(jlcxx::Module &mod) {

  // SeriesInterface
  {
    auto type = mod.add_type<SeriesInterface>(
        "CXX_SeriesInterface", jlcxx::julia_base_type<Attributable>());

    type.method("cxx_openPMD_version", &SeriesInterface::openPMD);
    type.method("cxx_set_openPMD_version!", &SeriesInterface::setOpenPMD);
    type.method("cxx_openPMD_extension", &SeriesInterface::openPMDextension);
    type.method("cxx_set_openPMD_extension!",
                &SeriesInterface::setOpenPMDextension);
    type.method("cxx_base_path", &SeriesInterface::basePath);
    type.method("cxx_set_base_path!", &SeriesInterface::setBasePath);
    type.method("cxx_meshes_path", &SeriesInterface::meshesPath);
    type.method("cxx_set_meshes_path!", &SeriesInterface::setMeshesPath);
    type.method("cxx_particles_path", &SeriesInterface::particlesPath);
    type.method("cxx_set_particles_path!", &SeriesInterface::setParticlesPath);
    type.method("cxx_author", &SeriesInterface::author);
    type.method("cxx_set_author!", &SeriesInterface::setAuthor);
    type.method("cxx_software", &SeriesInterface::software);
    type.method(
        "cxx_set_software!",
        static_cast<SeriesInterface &(SeriesInterface::*)(std::string const &,
                                                          std::string const &)>(
            &SeriesInterface::setSoftware));
    type.method("cxx_set_software!",
                (SeriesInterface & (SeriesInterface::*)(std::string const &))(
                    &SeriesInterface::setSoftware));
    type.method("cxx_software_version", &SeriesInterface::softwareVersion);
    // type.method("set_software_version!",
    // &SeriesInterface::setSoftwareVersion);
    type.method("cxx_date", &SeriesInterface::date);
    type.method("cxx_set_date!", &SeriesInterface::setDate);
    type.method("cxx_software_dependencies",
                &SeriesInterface::softwareDependencies);
    type.method("cxx_set_software_dependencies!",
                &SeriesInterface::setSoftwareDependencies);
    type.method("cxx_machine", &SeriesInterface::machine);
    type.method("cxx_set_machine!", &SeriesInterface::setMachine);
    // TODO: type.method("iteration_encoding",
    // &SeriesInterface::iterationEncoding);
    // TODO: type.method("set_iteration_encoding!",
    // &SeriesInterface::setIterationEncoding);
    type.method("cxx_iteration_format", &SeriesInterface::iterationFormat);
    type.method("cxx_set_iteration_format!",
                &SeriesInterface::setIterationFormat);
    type.method("cxx_name", &SeriesInterface::name);
    type.method("cxx_set_name!", &SeriesInterface::setName);
    type.method("cxx_backend", &SeriesInterface::backend);
    type.method("cxx_flush", &SeriesInterface::flush);
  }

  // Series

  {
    auto type = mod.add_type<Series>("CXX_Series",
                                     jlcxx::julia_base_type<SeriesInterface>());

    type.constructor<>();
#if openPMD_HAVE_MPI
    type.method("cxx_Series", [](const std::string &filepath, Access at,
                                 sized_uint_t<sizeof(MPI_Comm)> comm,
                                 const std::string &options) {
      return Series(filepath, at, *(const MPI_Comm *)&comm, options);
    });
    type.method("cxx_Series", [](const std::string &filepath, Access at,
                                 sized_uint_t<sizeof(MPI_Comm)> comm) {
      return Series(filepath, at, *(const MPI_Comm *)&comm);
    });
#endif
    type.constructor<const std::string &, Access, const std::string &>();
    type.constructor<const std::string &, Access>();

    type.method("cxx_isvalid",
                [](const Series &series) { return bool(series); });
    type.method("cxx_close", &Series::close);
    type.method("cxx_iterations",
                [](Series &series) -> Container<Iteration, uint64_t> & {
                  return series.iterations;
                });
    // TODO type.method("read_iterations", &Series::readIterations);
    type.method("cxx_write_iterations", &Series::writeIterations);
  }
}
