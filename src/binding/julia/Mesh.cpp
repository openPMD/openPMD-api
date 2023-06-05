// Mesh

#include "defs.hpp"

// Define supertype relationships
namespace jlcxx
{
template <>
struct SuperType<Mesh>
{
    using type = Container<MeshRecordComponent>;
};
} // namespace jlcxx

void define_julia_Mesh(jlcxx::Module &mod)
{

    // Mesh::Geometry
    mod.add_bits<Mesh::Geometry>("Geometry", jlcxx::julia_type("CppEnum"));
    jlcxx::stl::apply_stl<Mesh::Geometry>(mod);

    mod.set_const("GEOMETRY_cartesian", Mesh::Geometry::cartesian);
    mod.set_const("GEOMETRY_theta_mode", Mesh::Geometry::thetaMode);
    mod.set_const("GEOMETRY_cylindrical", Mesh::Geometry::cylindrical);
    mod.set_const("GEOMETRY_spherical", Mesh::Geometry::spherical);
    mod.set_const("GEOMETRY_other", Mesh::Geometry::other);

    // Mesh::DataOrder
    mod.add_bits<Mesh::DataOrder>("DataOrder", jlcxx::julia_type("CppEnum"));
    jlcxx::stl::apply_stl<Mesh::DataOrder>(mod);

    mod.set_const("DATAORDER_C", Mesh::DataOrder::C);
    mod.set_const("DATAORDER_F", Mesh::DataOrder::F);

    // Mesh
    auto type = mod.add_type<Mesh>(
        "CXX_Mesh",
        // We don't wrap BaseRecord<T> for simplicity. We thus need to declare
        // Container<MeshRecordComponent> as our supertype.
        jlcxx::julia_base_type<Container<MeshRecordComponent>>());

    // These two functions come from our superclass
    // BaseRecord<MeshRecordComponent>. We declare them as if they were our own.
    type.method("cxx_unit_dimension", &Mesh::unitDimension);
    type.method("cxx_isscalar", &Mesh::scalar);

    type.method("cxx_geometry", &Mesh::geometry);
    type.method(
        "cxx_set_geometry!",
        static_cast<Mesh &(Mesh::*)(Mesh::Geometry g)>(&Mesh::setGeometry));
    type.method("cxx_geometry_parameters", &Mesh::geometryParameters);
    type.method("cxx_set_geometry_parameters!", &Mesh::setGeometryParameters);
    type.method("cxx_data_order", &Mesh::dataOrder);
    type.method("cxx_set_data_order!", &Mesh::setDataOrder);
    type.method("cxx_axis_labels", &Mesh::axisLabels);
    type.method("cxx_set_axis_labels!", &Mesh::setAxisLabels);
    type.method("cxx_grid_spacing", &Mesh::gridSpacing<double>);
    type.method("cxx_set_grid_spacing!", &Mesh::setGridSpacing<double>);
    type.method("cxx_grid_global_offset", &Mesh::gridGlobalOffset);
    type.method("cxx_set_grid_global_offset!", &Mesh::setGridGlobalOffset);
    type.method("cxx_grid_unit_SI", &Mesh::gridUnitSI);
    type.method("cxx_set_grid_unit_SI!", &Mesh::setGridUnitSI);
    type.method(
        "cxx_set_unit_dimension!",
        [](Mesh &mesh, const array_double_7 &unitDimension) {
            return mesh.setUnitDimension(std::map<UnitDimension, double>{
                {UnitDimension::L, unitDimension[uint8_t(UnitDimension::L)]},
                {UnitDimension::M, unitDimension[uint8_t(UnitDimension::M)]},
                {UnitDimension::T, unitDimension[uint8_t(UnitDimension::T)]},
                {UnitDimension::I, unitDimension[uint8_t(UnitDimension::I)]},
                {UnitDimension::theta,
                 unitDimension[uint8_t(UnitDimension::theta)]},
                {UnitDimension::N, unitDimension[uint8_t(UnitDimension::N)]},
                {UnitDimension::J, unitDimension[uint8_t(UnitDimension::J)]},
            });
        });
    type.method("cxx_time_offset", &Mesh::timeOffset<double>);
    type.method("cxx_set_time_offset!", &Mesh::setTimeOffset<double>);
}
