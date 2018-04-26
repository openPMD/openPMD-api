#include <pybind11/pybind11.h>

#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"

#include <string>

namespace py = pybind11;
using namespace openPMD;


void init_BaseRecord(py::module &m) {
    py::class_<BaseRecord< MeshRecordComponent >, Container< MeshRecordComponent > >(m, "Mesh_Base_Record");

    py::enum_<UnitDimension>(m, "Unit_Dimension")
        .value("L", UnitDimension::L)
        .value("M", UnitDimension::M)
        .value("T", UnitDimension::T)
        .value("I", UnitDimension::I)
        .value("theta", UnitDimension::theta)
        .value("N", UnitDimension::N)
        .value("J", UnitDimension::J)
    ;
}
