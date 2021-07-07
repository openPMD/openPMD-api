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
        "SeriesImpl", jlcxx::julia_base_type<Attributable>());

    type.method("openPMD_version", &SeriesImpl::openPMD);
    type.method("set_openPMD_version!", &SeriesImpl::setOpenPMD);
    type.method("openPMD_extension", &SeriesImpl::openPMDextension);
    type.method("set_openPMD_extension!", &SeriesImpl::setOpenPMDextension);
    type.method("base_path", &SeriesImpl::basePath);
    type.method("set_base_path!", &SeriesImpl::setBasePath);
    type.method("meshes_path", &SeriesImpl::meshesPath);
    type.method("set_meshes_path!", &SeriesImpl::setMeshesPath);
    type.method("particles_path", &SeriesImpl::particlesPath);
    type.method("set_particles_path!", &SeriesImpl::setParticlesPath);
    type.method("author", &SeriesImpl::author);
    type.method("set_author!", &SeriesImpl::setAuthor);
    type.method("software", &SeriesImpl::software);
    type.method("set_software!",
                static_cast<SeriesImpl &(SeriesImpl::*)(std::string const &,
                                                        std::string const &)>(
                    &SeriesImpl::setSoftware));
    type.method("set_software!",
                (SeriesImpl & (SeriesImpl::*)(std::string const &))(
                    &SeriesImpl::setSoftware));
    type.method("software_version", &SeriesImpl::softwareVersion);
    // type.method("set_software_version!", &SeriesImpl::setSoftwareVersion);
    type.method("date", &SeriesImpl::date);
    type.method("set_date!", &SeriesImpl::setDate);
    type.method("software_dependencies", &SeriesImpl::softwareDependencies);
    type.method("set_software_dependencies!",
                &SeriesImpl::setSoftwareDependencies);
    type.method("machine", &SeriesImpl::machine);
    type.method("set_machine!", &SeriesImpl::setMachine);
    // TODO: type.method("iteration_encoding", &SeriesImpl::iterationEncoding);
    // TODO: type.method("set_iteration_encoding!",
    // &SeriesImpl::setIterationEncoding);
    type.method("iteration_format", &SeriesImpl::iterationFormat);
    type.method("set_iteration_format!", &SeriesImpl::setIterationFormat);
    type.method("name", &SeriesImpl::name);
    type.method("set_name!", &SeriesImpl::setName);
    type.method("backend", &SeriesImpl::backend);
    type.method("flush1", &SeriesImpl::flush);
  }

  // Series

  {
    auto type =
        mod.add_type<Series>("Series", jlcxx::julia_base_type<SeriesImpl>());

    type.constructor<>();
#if openPMD_HAVE_MPI
    type.constructor<const std::string &, Access, MPI_Comm,
                     const std::string &>();
    type.constructor<const std::string &, Access, MPI_Comm>();
#endif
    type.constructor<const std::string &, Access, const std::string &>();
    type.constructor<const std::string &, Access>();

    type.method("isvalid1", [](const Series &series) { return bool(series); });
    type.method("iterations",
                [](Series &series) -> Container<Iteration, uint64_t> & {
                  return series.iterations;
                });
    //TODO type.method("read_iterations", &Series::readIterations);
    type.method("write_iterations", &Series::writeIterations);
  }
}
