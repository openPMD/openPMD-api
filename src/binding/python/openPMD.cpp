#include <pybind11/pybind11.h>
//include <pybind11/stl.h>

#include "openPMD/version.hpp"

#include <sstream>

namespace py = pybind11;


// forward declarations of exposed classes
void init_AccessType(py::module &);
void init_BaseRecord(py::module &);
void init_BaseRecordComponent(py::module &);
void init_Container(py::module &);
void init_Dataset(py::module &);
void init_Datatype(py::module &);
void init_Iteration(py::module &);
void init_IterationEncoding(py::module &);
void init_Mesh(py::module &);
void init_MeshRecordComponent(py::module &);
void init_ParticlePatches(py::module &);
void init_ParticleSpecies(py::module &);
void init_Record(py::module &);
void init_RecordComponent(py::module &);
void init_Series(py::module &);


PYBIND11_MODULE(openPMD, m) {
    // m.doc() = ...;

    // note: order from parent to child classes
    init_AccessType(m);
    init_Container(m);
    init_BaseRecord(m);
    init_Dataset(m);
    init_Datatype(m);
    init_Iteration(m);
    init_IterationEncoding(m);
    init_Mesh(m);
    init_BaseRecordComponent(m);
    init_RecordComponent(m);
    init_MeshRecordComponent(m);
    init_ParticlePatches(m);
    init_ParticleSpecies(m);
    init_Record(m);
    init_Series(m);

    // build version
    std::stringstream openPMDapi;
    openPMDapi << OPENPMDAPI_VERSION_MAJOR << "."
               << OPENPMDAPI_VERSION_MINOR << "."
               << OPENPMDAPI_VERSION_PATCH;
    if( std::string( OPENPMDAPI_VERSION_LABEL ).size() > 0 )
        openPMDapi << "-" << OPENPMDAPI_VERSION_LABEL;
    m.attr("__version__") = openPMDapi.str();

    // variants
    m.attr("have_mpi") = bool(openPMD_HAVE_MPI);
    m.attr("have_hdf5") = bool(openPMD_HAVE_HDF5);
    m.attr("have_adios1") = bool(openPMD_HAVE_ADIOS1);
    m.attr("have_adios2") = bool(openPMD_HAVE_ADIOS2);
    // m.attr("have_json") = bool(openPMD_HAVE_JSON);
}

