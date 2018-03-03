#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#include "openPMD/Series.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/backend/Container.hpp"

namespace py = pybind11;
using namespace openPMD;

using PyIterationContainer = Container< Iteration, uint64_t >;
PYBIND11_MAKE_OPAQUE(PyIterationContainer)

void init_Series(py::module &m) {
    py::bind_map< PyIterationContainer >(m, "Iteration_Container");

    py::class_<Series>(m, "Series")

        // private
        //.def(py::init<std::string const &, AccessType>())

        .def_static("create", [](std::string const & filepath){ return Series::create(filepath); })
        .def_static("read", [](std::string const & filepath){ return Series::read(filepath); })

        .def_property_readonly("openPMD", &Series::openPMD)
        .def("set_openPMD", &Series::setOpenPMD)
        .def_property_readonly("openPMD_extension", &Series::openPMDextension)
        .def("set_openPMD_extension", &Series::setOpenPMDextension)
        .def_property_readonly("base_ath", &Series::basePath)
        .def("set_base_path", &Series::setBasePath)
        .def_property_readonly("meshes_path", &Series::meshesPath)
        .def("set_meshes_path", &Series::setMeshesPath)
        .def_property_readonly("particles_path", &Series::particlesPath)
        .def("set_particles_path", &Series::setParticlesPath)
        .def_property_readonly("author", &Series::author)
        .def("set_author", &Series::setAuthor)
        .def_property_readonly("software", &Series::software)
        .def("set_software", &Series::setSoftware)
        .def_property_readonly("software_version", &Series::softwareVersion)
        .def("set_software_version", &Series::setSoftwareVersion)
        // softwareDependencies
        // machine
        .def_property_readonly("date", &Series::date)
        .def("set_date", &Series::setDate)
        .def_property_readonly("iteration_encoding", &Series::iterationEncoding)
        .def("set_iteration_encoding", &Series::setIterationEncoding)
        .def_property_readonly("iteration_format", &Series::iterationFormat)
        .def("set_iteration_format", &Series::setIterationFormat)
        .def_property_readonly("name", &Series::name)
        .def("set_name", &Series::setName)
        .def("flush", &Series::flush)

        .def_readwrite("iterations", &Series::iterations)
    ;
}
