#define BOOST_TEST_MODULE libopenpmd_read_test


#include <boost/test/included/unit_test.hpp>

/* make Writable::parent visible for hierarchy check */
#define protected public
#include "../include/Output.hpp"


BOOST_AUTO_TEST_CASE(git_hdf5_sample_structure_test)
{
    Output o = Output("../samples/git-sample/",
                      "data00000100.h5",
                      Output::IterationEncoding::fileBased,
                      Format::HDF5,
                      AccessType::READ_ONLY);

    BOOST_TEST(o.parent == nullptr);
    BOOST_TEST(o.iterations.parent == static_cast< Writable* >(&o));
    BOOST_TEST(o.iterations[100].parent == static_cast< Writable* >(&o.iterations));
    BOOST_TEST(o.iterations[100].meshes.parent == static_cast< Writable* >(&o.iterations[100]));
    BOOST_TEST(o.iterations[100].meshes["E"].parent == static_cast< Writable* >(&o.iterations[100].meshes));
    BOOST_TEST(o.iterations[100].meshes["E"]["x"].parent == static_cast< Writable* >(&o.iterations[100].meshes["E"]));
    BOOST_TEST(o.iterations[100].meshes["E"]["y"].parent == static_cast< Writable* >(&o.iterations[100].meshes["E"]));
    BOOST_TEST(o.iterations[100].meshes["E"]["z"].parent == static_cast< Writable* >(&o.iterations[100].meshes["E"]));
    BOOST_TEST(o.iterations[100].meshes["rho"].parent == static_cast< Writable* >(&o.iterations[100].meshes));
    BOOST_TEST(o.iterations[100].meshes["rho"][MeshRecordComponent::SCALAR].parent == static_cast< Writable* >(&o.iterations[100].meshes));
    BOOST_TEST(o.iterations[100].particles.parent == static_cast< Writable* >(&o.iterations[100]));
    BOOST_TEST(o.iterations[100].particles["electrons"].parent == static_cast< Writable* >(&o.iterations[100].particles));
    BOOST_TEST(o.iterations[100].particles["electrons"]["charge"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["charge"][RecordComponent::SCALAR].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["mass"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["mass"][RecordComponent::SCALAR].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["momentum"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["momentum"]["x"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["momentum"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["momentum"]["y"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["momentum"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["momentum"]["z"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["momentum"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["position"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["position"]["x"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["position"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["position"]["y"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["position"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["position"]["z"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["position"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["positionOffset"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["positionOffset"]["x"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["positionOffset"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["positionOffset"]["y"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["positionOffset"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["positionOffset"]["z"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]["positionOffset"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["weighting"].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
    BOOST_TEST(o.iterations[100].particles["electrons"]["weighting"][RecordComponent::SCALAR].parent == static_cast< Writable* >(&o.iterations[100].particles["electrons"]));
}

BOOST_AUTO_TEST_CASE(git_hdf5_sample_attribute_test)
{
    //TODO check non-standard attributes
    Output o = Output("../samples/git-sample/",
                      "data00000100.h5",
                      Output::IterationEncoding::fileBased,
                      Format::HDF5,
                      AccessType::READ_ONLY);

    BOOST_TEST(o.openPMD() == "1.0.0");
    BOOST_TEST(o.openPMDextension() == 1);
    BOOST_TEST(o.basePath() == "/data/%T/");
    BOOST_TEST(o.meshesPath() == "fields/");
    BOOST_TEST(o.particlesPath() == "particles/");
    BOOST_TEST(o.iterationEncoding() == Output::IterationEncoding::fileBased);
    BOOST_TEST(o.iterationFormat() == "data%T.h5");
    BOOST_TEST(o.name() == "data00000100.h5");

    BOOST_TEST(o.iterations.size() == 1);
    BOOST_TEST(o.iterations.count(100) == 1);

    Iteration& iteration_100 = o.iterations[100];
    BOOST_TEST(iteration_100.time< double >() == static_cast< double >(3.2847121452090077e-14));
    BOOST_TEST(iteration_100.dt< double >() == static_cast< double >(3.2847121452090093e-16));
    BOOST_TEST(iteration_100.timeUnitSI() == static_cast< double >(1));

    BOOST_TEST(iteration_100.meshes.size() == 2);
    BOOST_TEST(iteration_100.meshes.count("E") == 1);
    BOOST_TEST(iteration_100.meshes.count("rho") == 1);

    std::vector< std::string > al{"x", "y", "z"};
    std::vector< double > gs{8.0000000000000007e-07,
                             8.0000000000000007e-07,
                             1.0000000000000001e-07};
    std::vector< double > ggo{-1.0000000000000001e-05,
                              -1.0000000000000001e-05,
                              -5.1999999999999993e-06};
    std::array< double, 7 > ud{{1.,  1., -3., -1.,  0.,  0.,  0.}};
    Mesh& E = iteration_100.meshes["E"];
    BOOST_TEST(E.geometry() == Mesh::Geometry::cartesian);
    BOOST_TEST(E.dataOrder() == Mesh::DataOrder::C);
    BOOST_TEST(E.axisLabels() == al);
    BOOST_TEST(E.gridSpacing< double >() == gs);
    BOOST_TEST(E.gridGlobalOffset() == ggo);
    BOOST_TEST(E.gridUnitSI() == 1.0);
    BOOST_TEST(E.unitDimension() == ud);
    BOOST_TEST(E.timeOffset< double >() == static_cast< double >(0.0));

    BOOST_TEST(E.size() == 3);
    BOOST_TEST(E.count("x") == 1);
    BOOST_TEST(E.count("y") == 1);
    BOOST_TEST(E.count("z") == 1);

    std::vector< double > p{static_cast< double >(0.5),
                            static_cast< double >(0.),
                           static_cast< double >(0.)};
    Extent e{26, 26, 201};
    MeshRecordComponent& E_x = E["x"];
    BOOST_TEST(E_x.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(E_x.position< double >() == p);
    BOOST_TEST(E_x.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(E_x.getExtent() == e);
    BOOST_TEST(E_x.getDimensionality() == 3);

    p = {static_cast< double >(0.),
         static_cast< double >(0.5),
         static_cast< double >(0.)};
    MeshRecordComponent& E_y = E["y"];
    BOOST_TEST(E_y.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(E_y.position< double >() == p);
    BOOST_TEST(E_y.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(E_y.getExtent() == e);
    BOOST_TEST(E_y.getDimensionality() == 3);

    p = {static_cast< double >(0.),
         static_cast< double >(0.),
         static_cast< double >(0.5)};
    MeshRecordComponent& E_z = E["z"];
    BOOST_TEST(E_z.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(E_z.position< double >() == p);
    BOOST_TEST(E_z.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(E_z.getExtent() == e);
    BOOST_TEST(E_z.getDimensionality() == 3);

    gs = {8.0000000000000007e-07,
          8.0000000000000007e-07,
          1.0000000000000001e-07};
    ggo = {-1.0000000000000001e-05,
           -1.0000000000000001e-05,
           -5.1999999999999993e-06};
    ud = {{-3.,  0.,  1.,  1.,  0.,  0.,  0.}};
    Mesh& rho = iteration_100.meshes["rho"];
    BOOST_TEST(rho.geometry() == Mesh::Geometry::cartesian);
    BOOST_TEST(rho.dataOrder() == Mesh::DataOrder::C);
    BOOST_TEST(rho.axisLabels() == al);
    BOOST_TEST(rho.gridSpacing< double >() == gs);
    BOOST_TEST(rho.gridGlobalOffset() == ggo);
    BOOST_TEST(rho.gridUnitSI() == 1.0);
    BOOST_TEST(rho.unitDimension() == ud);
    BOOST_TEST(rho.timeOffset< double >() == static_cast< double >(0.0));

    BOOST_TEST(rho.size() == 1);
    BOOST_TEST(rho.count(MeshRecordComponent::SCALAR) == 1);

    p = {static_cast< double >(0.),
         static_cast< double >(0.),
         static_cast< double >(0.)};
    e = {26, 26, 201};
    MeshRecordComponent& rho_scalar = rho[MeshRecordComponent::SCALAR];
    BOOST_TEST(rho_scalar.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(rho_scalar.position< double >() == p);
    BOOST_TEST(rho_scalar.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(rho_scalar.getExtent() == e);
    BOOST_TEST(rho_scalar.getDimensionality() == 3);

    BOOST_TEST(iteration_100.particles.size() == 1);
    BOOST_TEST(iteration_100.particles.count("electrons") == 1);

    ParticleSpecies& electrons = iteration_100.particles["electrons"];

    BOOST_TEST(electrons.size() == 6);
    BOOST_TEST(electrons.count("charge") == 1);
    BOOST_TEST(electrons.count("mass") == 1);
    BOOST_TEST(electrons.count("momentum") == 1);
    BOOST_TEST(electrons.count("position") == 1);
    BOOST_TEST(electrons.count("positionOffset") == 1);
    BOOST_TEST(electrons.count("weighting") == 1);

    ud = {{0.,  0.,  1.,  1.,  0.,  0.,  0.}};
    Record& charge = electrons["charge"];
    BOOST_TEST(charge.unitDimension() == ud);
    BOOST_TEST(charge.timeOffset< double >() == static_cast< double >(0.0));

    BOOST_TEST(charge.size() == 1);
    BOOST_TEST(charge.count(RecordComponent::SCALAR) == 1);

    e = {85000};
    RecordComponent& charge_scalar = charge[RecordComponent::SCALAR];
    BOOST_TEST(charge_scalar.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(charge_scalar.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(charge_scalar.getDimensionality() == 1);
    BOOST_TEST(charge_scalar.getExtent() == e);

    ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
    Record& mass = electrons["mass"];
    BOOST_TEST(mass.unitDimension() == ud);
    BOOST_TEST(mass.timeOffset< double >() == static_cast< double >(0.0));

    BOOST_TEST(mass.size() == 1);
    BOOST_TEST(mass.count(RecordComponent::SCALAR) == 1);

    RecordComponent& mass_scalar = mass[RecordComponent::SCALAR];
    BOOST_TEST(mass_scalar.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(mass_scalar.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(mass_scalar.getDimensionality() == 1);
    BOOST_TEST(mass_scalar.getExtent() == e);

    ud = {{1.,  1., -1.,  0.,  0.,  0.,  0.}};
    Record& momentum = electrons["momentum"];
    BOOST_TEST(momentum.unitDimension() == ud);
    BOOST_TEST(momentum.timeOffset< double >() == static_cast< double >(0.0));

    BOOST_TEST(momentum.size() == 3);
    BOOST_TEST(momentum.count("x") == 1);
    BOOST_TEST(momentum.count("y") == 1);
    BOOST_TEST(momentum.count("z") == 1);

    RecordComponent& momentum_x = momentum["x"];
    BOOST_TEST(momentum_x.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(momentum_x.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(momentum_x.getDimensionality() == 1);
    BOOST_TEST(momentum_x.getExtent() == e);

    RecordComponent& momentum_y = momentum["y"];
    BOOST_TEST(momentum_y.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(momentum_y.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(momentum_y.getDimensionality() == 1);
    BOOST_TEST(momentum_y.getExtent() == e);

    RecordComponent& momentum_z = momentum["z"];
    BOOST_TEST(momentum_z.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(momentum_z.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(momentum_z.getDimensionality() == 1);
    BOOST_TEST(momentum_z.getExtent() == e);

    ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
    Record& position = electrons["position"];
    BOOST_TEST(position.unitDimension() == ud);
    BOOST_TEST(position.timeOffset< double >() == static_cast< double >(0.0));

    BOOST_TEST(position.size() == 3);
    BOOST_TEST(position.count("x") == 1);
    BOOST_TEST(position.count("y") == 1);
    BOOST_TEST(position.count("z") == 1);

    RecordComponent& position_x = position["x"];
    BOOST_TEST(position_x.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(position_x.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(position_x.getDimensionality() == 1);
    BOOST_TEST(position_x.getExtent() == e);

    RecordComponent& position_y = position["y"];
    BOOST_TEST(position_y.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(position_y.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(position_y.getDimensionality() == 1);
    BOOST_TEST(position_y.getExtent() == e);

    RecordComponent& position_z = position["z"];
    BOOST_TEST(position_z.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(position_z.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(position_z.getDimensionality() == 1);
    BOOST_TEST(position_z.getExtent() == e);

    Record& positionOffset = electrons["positionOffset"];
    BOOST_TEST(positionOffset.unitDimension() == ud);
    BOOST_TEST(positionOffset.timeOffset< double >() == static_cast< double >(0.0));

    BOOST_TEST(positionOffset.size() == 3);
    BOOST_TEST(positionOffset.count("x") == 1);
    BOOST_TEST(positionOffset.count("y") == 1);
    BOOST_TEST(positionOffset.count("z") == 1);

    RecordComponent& positionOffset_x = positionOffset["x"];
    BOOST_TEST(positionOffset_x.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(positionOffset_x.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(positionOffset_x.getDimensionality() == 1);
    BOOST_TEST(positionOffset_x.getExtent() == e);

    RecordComponent& positionOffset_y = positionOffset["y"];
    BOOST_TEST(positionOffset_y.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(positionOffset_y.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(positionOffset_y.getDimensionality() == 1);
    BOOST_TEST(positionOffset_y.getExtent() == e);

    RecordComponent& positionOffset_z = positionOffset["z"];
    BOOST_TEST(positionOffset_z.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(positionOffset_z.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(positionOffset_z.getDimensionality() == 1);
    BOOST_TEST(positionOffset_z.getExtent() == e);

    ud = {{0.,  0.,  0.,  0.,  0.,  0.,  0.}};
    Record& weighting = electrons["weighting"];
    BOOST_TEST(weighting.unitDimension() == ud);
    BOOST_TEST(weighting.timeOffset< double >() == static_cast< double >(0.0));

    BOOST_TEST(weighting.size() == 1);
    BOOST_TEST(weighting.count(RecordComponent::SCALAR) == 1);

    RecordComponent& weighting_scalar = weighting[RecordComponent::SCALAR];
    BOOST_TEST(weighting_scalar.unitSI() == static_cast< double >(1.0));
    BOOST_TEST(weighting_scalar.getDatatype() == Datatype::DOUBLE);
    BOOST_TEST(weighting_scalar.getDimensionality() == 1);
    BOOST_TEST(weighting_scalar.getExtent() == e);
}

BOOST_AUTO_TEST_CASE(hzdr_hdf5_sample_structure_test)
{
    /*
    Output o = Output("../samples/git-sample/",
                      "data00000100.h5",
                      Output::IterationEncoding::fileBased,
                      Format::HDF5,
                      AccessType::READ_ONLY);
                      */

}