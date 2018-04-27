#include <pybind11/pybind11.h>
#include <pybind11/stl_bind.h>

#include "openPMD/backend/Container.hpp"
//include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;

using PyIterationContainer = Container< Iteration, uint64_t >;
using PyMeshContainer = Container< Mesh >;
using PyPartContainer = Container< ParticleSpecies >;
using PyMeshRecordComponentContainer = Container< MeshRecordComponent >;
PYBIND11_MAKE_OPAQUE(PyIterationContainer)
PYBIND11_MAKE_OPAQUE(PyMeshContainer)
PYBIND11_MAKE_OPAQUE(PyPartContainer)
PYBIND11_MAKE_OPAQUE(PyMeshRecordComponentContainer)


void init_Container(py::module &m) {
    py::bind_map< PyIterationContainer >(m, "Iteration_Container");
    py::bind_map< PyMeshContainer >(m, "Mesh_Container");
    py::bind_map< PyPartContainer >(m, "Particle_Container");
    py::bind_map< PyMeshRecordComponentContainer >(m, "Mesh_Record_Component_Container");

}
