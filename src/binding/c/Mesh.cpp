#include <openPMD/binding/c/Mesh.h>

#include <openPMD/binding/c/UnitDimension.h>

#include <openPMD/Mesh.hpp>

#include <stdlib.h>
#include <string.h>

#include <map>
#include <string>
#include <utility>
#include <vector>

const openPMD_Container_MeshRecordComponent *
openPMD_Mesh_getConstContainer_MeshRecordComponent(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_container =
        (const openPMD::Container<openPMD::MeshRecordComponent> *)cxx_mesh;
    const auto container =
        (const openPMD_Container_MeshRecordComponent *)cxx_container;
    return container;
}

openPMD_Container_MeshRecordComponent *
openPMD_Mesh_getContainer_MeshRecordComponent(openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    const auto cxx_container =
        (openPMD::Container<openPMD::MeshRecordComponent> *)cxx_mesh;
    const auto container =
        (openPMD_Container_MeshRecordComponent *)cxx_container;
    return container;
}

// From BaseRecord<MeshRecordComponent> (too lazy to wrap this class)
openPMD_Array7 openPMD_Mesh_unitDimension(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_unitDimension = cxx_mesh->unitDimension();
    openPMD_Array7 unitDimension;
    for (size_t n = 0; n < 7; ++n)
        unitDimension.element[n] = cxx_unitDimension[n];
    return unitDimension;
}

bool openPMD_Mesh_scalar(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    return cxx_mesh->scalar();
}

// From Mesh
openPMD_Mesh *openPMD_Mesh_copy(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_mesh_copy = new openPMD::Mesh(*cxx_mesh);
    const auto mesh_copy = (openPMD_Mesh *)cxx_mesh_copy;
    return mesh_copy;
}

void openPMD_Mesh_delete(openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    delete cxx_mesh;
}

openPMD_Mesh_Geometry openPMD_Mesh_geometry(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_geometry = cxx_mesh->geometry();
    const auto geometry = openPMD_Mesh_Geometry(cxx_geometry);
    return geometry;
}

const char *openPMD_Mesh_geometryString(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_geometry = cxx_mesh->geometry();
    const auto geometry = openPMD_Mesh_Geometry(cxx_geometry);
    switch (geometry)
    {
    case openPMD_Mesh_Geometry_cartesian:
        return "cartesian";
    case openPMD_Mesh_Geometry_thetaMode:
        return "thetaMode";
    case openPMD_Mesh_Geometry_cylindrical:
        return "cylindrical";
    case openPMD_Mesh_Geometry_spherical:
        return "spherical";
    case openPMD_Mesh_Geometry_other:
        return "other";
    }
    abort();
}

void openPMD_Mesh_setGeometry(
    openPMD_Mesh *mesh, openPMD_Mesh_Geometry geometry)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    const auto cxx_geometry = openPMD::Mesh::Geometry(geometry);
    cxx_mesh->setGeometry(cxx_geometry);
}

char *openPMD_Mesh_geometryParameters(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_parameters = cxx_mesh->geometryParameters();
    return strdup(cxx_parameters.c_str());
}

void openPMD_Mesh_setGeometryParameters(
    openPMD_Mesh *mesh, const char *geometryParameters)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    cxx_mesh->setGeometryParameters(std::string(geometryParameters));
}

openPMD_Mesh_DataOrder openPMD_Mesh_dataOrder(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_dataOrder = cxx_mesh->dataOrder();
    const auto dataOrder = openPMD_Mesh_DataOrder(cxx_dataOrder);
    return dataOrder;
}

void openPMD_Mesh_setDataOrder(
    openPMD_Mesh *mesh, openPMD_Mesh_DataOrder dataOrder)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    const auto cxx_dataOrder = openPMD::Mesh::DataOrder(dataOrder);
    cxx_mesh->setDataOrder(cxx_dataOrder);
}

char **openPMD_Mesh_axisLabels(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_axisLabels = cxx_mesh->axisLabels();
    const size_t size = cxx_axisLabels.size();
    char **axisLabels = (char **)malloc(size * sizeof *axisLabels);
    for (size_t d = 0; d < cxx_axisLabels.size(); ++d)
        axisLabels[d] = strdup(cxx_axisLabels[d].c_str());
    return axisLabels;
}

size_t openPMD_Mesh_axisLabelsSize(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_axisLabels = cxx_mesh->axisLabels();
    return cxx_axisLabels.size();
}

void openPMD_Mesh_setAxisLabels(
    openPMD_Mesh *mesh, const char *const *axisLabels, size_t size)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    std::vector<std::string> cxx_axisLabels(size);
    for (size_t d = 0; d < size; ++d)
        cxx_axisLabels[d] = std::string(axisLabels[d]);
    cxx_mesh->setAxisLabels(std::move(cxx_axisLabels));
}

double *openPMD_Mesh_gridSpacing(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_gridSpacing = cxx_mesh->gridSpacing<double>();
    const size_t size = cxx_gridSpacing.size();
    double *gridSpacing = (double *)malloc(size * sizeof *gridSpacing);
    for (size_t d = 0; d < size; ++d)
        gridSpacing[d] = cxx_gridSpacing[d];
    return gridSpacing;
}

size_t openPMD_Mesh_gridSpacingSize(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_gridSpacing = cxx_mesh->gridSpacing<double>();
    return cxx_gridSpacing.size();
}

void openPMD_Mesh_setgridSpacing(
    openPMD_Mesh *mesh, const double gridSpacing, size_t size)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    std::vector<double> cxx_gridSpacing(gridSpacing, gridSpacing + size);
    cxx_mesh->setGridSpacing(std::move(cxx_gridSpacing));
}

double *openPMD_Mesh_gridGlobalOffset(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_gridGlobalOffset = cxx_mesh->gridGlobalOffset();
    const size_t size = cxx_gridGlobalOffset.size();
    double *gridGlobalOffset =
        (double *)malloc(size * sizeof *gridGlobalOffset);
    for (size_t d = 0; d < size; ++d)
        gridGlobalOffset[d] = cxx_gridGlobalOffset[d];
    return gridGlobalOffset;
}

size_t openPMD_Mesh_gridGlobalOffsetSize(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    const auto cxx_gridGlobalOffset = cxx_mesh->gridGlobalOffset();
    return cxx_gridGlobalOffset.size();
}

void openPMD_Mesh_setgridGlobalOffset(
    openPMD_Mesh *mesh, const double gridGlobalOffset, size_t size)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    std::vector<double> cxx_gridGlobalOffset(
        gridGlobalOffset, gridGlobalOffset + size);
    cxx_mesh->setGridGlobalOffset(std::move(cxx_gridGlobalOffset));
}

double openPMD_Mesh_gridUnitSI(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    return cxx_mesh->gridUnitSI();
}

void openPMD_Mesh_setGridUnitSI(openPMD_Mesh *mesh, double unitSI)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    cxx_mesh->setGridUnitSI(unitSI);
}

void openPMD_Mesh_setUnitDimension(
    openPMD_Mesh *mesh, const openPMD_Array7 *unitDimension)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    std::map<openPMD::UnitDimension, double> cxx_unitDimension;
    for (size_t n = 0; n < 7; ++n)
        cxx_unitDimension[openPMD::UnitDimension(n)] =
            unitDimension->element[n];
    cxx_mesh->setUnitDimension(std::move(cxx_unitDimension));
}

double openPMD_Mesh_timeOffset(const openPMD_Mesh *mesh)
{
    const auto cxx_mesh = (const openPMD::Mesh *)mesh;
    return cxx_mesh->timeOffset<double>();
}

void openPMD_Mesh_setTimeOffset(openPMD_Mesh *mesh, double timeOffset)
{
    const auto cxx_mesh = (openPMD::Mesh *)mesh;
    cxx_mesh->setTimeOffset(timeOffset);
}
