#include <pybind11/pybind11.h>

namespace py = pybind11;


// forward declarations of exposed classes
void init_Series(py::module &);
void init_Dataset(py::module &);
/*
void init_Datatype(py::module &);
void init_Iteration(py::module &);
void init_IterationEncoding(py::module &);
void init_Mesh(py::module &);
void init_ParticlePatches(py::module &);
void init_ParticleSpecies(py::module &);
void init_Record(py::module &);
void init_RecordComponent(py::module &);
*/


PYBIND11_MODULE(openPMD, m) {
    // m.doc() = ...;

    init_Series(m);
    init_Dataset(m);
    /*
    init_Datatype(m);
    init_Iteration(m);
    init_IterationEncoding(m);
    init_Mesh(m);
    init_ParticlePatches(m);
    init_ParticleSpecies(m);
    init_Record(m);
    init_RecordComponent(m);
    */

    // m.attr("__version__") = "1.2.3-dev";
}

