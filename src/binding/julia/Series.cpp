// Series

#include "defs.hpp"

// Define supertype relationships
namespace jlcxx {
template <> struct SuperType<SeriesImpl> { typedef Attributable type; };
template <> struct SuperType<Series> { typedef SeriesImpl type; };
} // namespace jlcxx

void define_julia_Series(jlcxx::Module &mod) {

  // SeriesImpl
  {
    auto type = mod.add_type<SeriesImpl>(
        "CXX_SeriesImpl", jlcxx::julia_base_type<Attributable>());

    type.method("cxx_openPMD_version", &SeriesImpl::openPMD);
    type.method("cxx_set_openPMD_version!", &SeriesImpl::setOpenPMD);
    type.method("cxx_openPMD_extension", &SeriesImpl::openPMDextension);
    type.method("cxx_set_openPMD_extension!", &SeriesImpl::setOpenPMDextension);
    type.method("cxx_base_path", &SeriesImpl::basePath);
    type.method("cxx_set_base_path!", &SeriesImpl::setBasePath);
    type.method("cxx_meshes_path", &SeriesImpl::meshesPath);
    type.method("cxx_set_meshes_path!", &SeriesImpl::setMeshesPath);
    type.method("cxx_particles_path", &SeriesImpl::particlesPath);
    type.method("cxx_set_particles_path!", &SeriesImpl::setParticlesPath);
    type.method("cxx_author", &SeriesImpl::author);
    type.method("cxx_set_author!", &SeriesImpl::setAuthor);
    type.method("cxx_software", &SeriesImpl::software);
    type.method("cxx_set_software!",
                static_cast<SeriesImpl &(SeriesImpl::*)(std::string const &,
                                                        std::string const &)>(
                    &SeriesImpl::setSoftware));
    type.method("cxx_set_software!",
                (SeriesImpl & (SeriesImpl::*)(std::string const &))(
                    &SeriesImpl::setSoftware));
    type.method("cxx_software_version", &SeriesImpl::softwareVersion);
    // type.method("set_software_version!", &SeriesImpl::setSoftwareVersion);
    type.method("cxx_date", &SeriesImpl::date);
    type.method("cxx_set_date!", &SeriesImpl::setDate);
    type.method("cxx_software_dependencies", &SeriesImpl::softwareDependencies);
    type.method("cxx_set_software_dependencies!",
                &SeriesImpl::setSoftwareDependencies);
    type.method("cxx_machine", &SeriesImpl::machine);
    type.method("cxx_set_machine!", &SeriesImpl::setMachine);
    // TODO: type.method("iteration_encoding", &SeriesImpl::iterationEncoding);
    // TODO: type.method("set_iteration_encoding!",
    // &SeriesImpl::setIterationEncoding);
    type.method("cxx_iteration_format", &SeriesImpl::iterationFormat);
    type.method("cxx_set_iteration_format!", &SeriesImpl::setIterationFormat);
    type.method("cxx_name", &SeriesImpl::name);
    type.method("cxx_set_name!", &SeriesImpl::setName);
    type.method("cxx_backend", &SeriesImpl::backend);
    type.method("cxx_flush", &SeriesImpl::flush);
  }

  // Series

  {
    auto type = mod.add_type<Series>("CXX_Series",
                                     jlcxx::julia_base_type<SeriesImpl>());

    type.constructor<>();
#if openPMD_HAVE_MPI
    type.constructor<const std::string &, Access, MPI_Comm,
                     const std::string &>();
    type.constructor<const std::string &, Access, MPI_Comm>();
#endif
    type.constructor<const std::string &, Access, const std::string &>();
    type.constructor<const std::string &, Access>();

    type.method("cxx_isvalid",
                [](const Series &series) { return bool(series); });
    type.method("cxx_iterations",
                [](Series &series) -> Container<Iteration, uint64_t> & {
                  return series.iterations;
                });
    // TODO type.method("read_iterations", &Series::readIterations);
    type.method("cxx_write_iterations", &Series::writeIterations);
  }
}
