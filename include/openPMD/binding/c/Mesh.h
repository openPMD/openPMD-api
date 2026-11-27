#ifndef OPENPMD_MESH_H
#define OPENPMD_MESH_H

#include <openPMD/binding/c/Mesh.h>

#include <openPMD/binding/c/UnitDimension.h>
#include <openPMD/binding/c/backend/Container_MeshRecordComponent.h>

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct openPMD_Mesh openPMD_Mesh;

    const openPMD_Container_MeshRecordComponent *
    openPMD_Mesh_getConstContainer_MeshRecordComponent(
        const openPMD_Mesh *mesh);

    openPMD_Container_MeshRecordComponent *
    openPMD_Mesh_getContainer_MeshRecordComponent(openPMD_Mesh *mesh);

    // From BaseRecord<MeshRecordComponent> (too lazy to wrap this class)
    openPMD_ArrayDouble7 openPMD_Mesh_unitDimension(const openPMD_Mesh *mesh);

    bool openPMD_Mesh_scalar(const openPMD_Mesh *mesh);

    // From Mesh
    openPMD_Mesh *openPMD_Mesh_copy(const openPMD_Mesh *mesh);

    void openPMD_Mesh_delete(openPMD_Mesh *mesh);

    typedef enum openPMD_Mesh_Geometry
    {
        openPMD_Mesh_Geometry_cartesian,
        openPMD_Mesh_Geometry_thetaMode,
        openPMD_Mesh_Geometry_cylindrical,
        openPMD_Mesh_Geometry_spherical,
        openPMD_Mesh_Geometry_other
    } openPMD_Mesh_Geometry;

    typedef enum openPMD_Mesh_DataOrder
    {
        openPMD_Mesh_DataOrder_C = 'C',
        openPMD_Mesh_DataOrder_F = 'F'
    } openPMD_Mesh_DataOrder;

    openPMD_Mesh_Geometry openPMD_Mesh_geometry(const openPMD_Mesh *mesh);

    const char *openPMD_Mesh_geometryString(const openPMD_Mesh *mesh);

    void openPMD_Mesh_setGeometry(
        openPMD_Mesh *mesh, openPMD_Mesh_Geometry geometry);

    char *openPMD_Mesh_geometryParameters(const openPMD_Mesh *mesh);

    void openPMD_Mesh_setGeometryParameters(
        openPMD_Mesh *mesh, const char *geometryParameters);

    openPMD_Mesh_DataOrder openPMD_Mesh_dataOrder(const openPMD_Mesh *mesh);

    void openPMD_Mesh_setDataOrder(
        openPMD_Mesh *mesh, openPMD_Mesh_DataOrder dataOrder);

    char **openPMD_Mesh_axisLabels(const openPMD_Mesh *mesh);

    size_t openPMD_Mesh_axisLabelsSize(const openPMD_Mesh *mesh);

    void openPMD_Mesh_setAxisLabels(
        openPMD_Mesh *mesh, const char *const *axisLabels, size_t size);

    double *openPMD_Mesh_gridSpacing(const openPMD_Mesh *mesh);

    size_t openPMD_Mesh_gridSpacingSize(const openPMD_Mesh *mesh);

    void openPMD_Mesh_setgridSpacing(
        openPMD_Mesh *mesh, const double *gridSpacing, size_t size);

    double *openPMD_Mesh_gridGlobalOffset(const openPMD_Mesh *mesh);

    size_t openPMD_Mesh_gridGlobalOffsetSize(const openPMD_Mesh *mesh);

    void openPMD_Mesh_setgridGlobalOffset(
        openPMD_Mesh *mesh, const double *gridGlobalOffset, size_t size);

    double openPMD_Mesh_gridUnitSI(const openPMD_Mesh *mesh);

    void openPMD_Mesh_setGridUnitSI(openPMD_Mesh *mesh, double unitSI);

    void openPMD_Mesh_setUnitDimension(
        openPMD_Mesh *mesh, const openPMD_ArrayDouble7 *unitDimension);

    double openPMD_Mesh_timeOffset(const openPMD_Mesh *mesh);

    void openPMD_Mesh_setTimeOffset(openPMD_Mesh *mesh, double timeOffset);

#ifdef __cplusplus
}
#endif

#endif // #ifndef OPENPMD_MESH_H
