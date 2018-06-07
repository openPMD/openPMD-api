#define CATCH_CONFIG_MAIN


#if openPMD_HAVE_INVASIVE_TESTS
/* make Writable::parent visible for hierarchy check */
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wkeyword-macro"
#   define protected public
#   pragma clang diagnostic pop
#endif
#include "openPMD/openPMD.hpp"
#if openPMD_HAVE_INVASIVE_TESTS
#   undef protected
#endif
using namespace openPMD;

#include <catch/catch.hpp>

#include <iostream>
#include <algorithm>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <limits>


#if openPMD_HAVE_HDF5
TEST_CASE( "git_hdf5_sample_structure_test", "[serial][hdf5]" )
{
#if openPMD_HAVE_INVASIVE_TESTS
    try
    {
        Series o = Series("../samples/git-sample/data%T.h5", AccessType::READ_ONLY);

        REQUIRE(!o.parent);
        REQUIRE(o.iterations.parent == getWritable(&o));
        REQUIRE_THROWS_AS(o.iterations[42], std::out_of_range);
        REQUIRE(o.iterations[100].parent == getWritable(&o.iterations));
        REQUIRE(o.iterations[100].meshes.parent == getWritable(&o.iterations[100]));
        REQUIRE(o.iterations[100].meshes["E"].parent == getWritable(&o.iterations[100].meshes));
        REQUIRE(o.iterations[100].meshes["E"]["x"].parent == getWritable(&o.iterations[100].meshes["E"]));
        REQUIRE(o.iterations[100].meshes["E"]["y"].parent == getWritable(&o.iterations[100].meshes["E"]));
        REQUIRE(o.iterations[100].meshes["E"]["z"].parent == getWritable(&o.iterations[100].meshes["E"]));
        REQUIRE(o.iterations[100].meshes["rho"].parent == getWritable(&o.iterations[100].meshes));
        REQUIRE(o.iterations[100].meshes["rho"][MeshRecordComponent::SCALAR].parent == getWritable(&o.iterations[100].meshes));
        REQUIRE_THROWS_AS(o.iterations[100].meshes["cherries"], std::out_of_range);
        REQUIRE(o.iterations[100].particles.parent == getWritable(&o.iterations[100]));
        REQUIRE(o.iterations[100].particles["electrons"].parent == getWritable(&o.iterations[100].particles));
        REQUIRE(o.iterations[100].particles["electrons"]["charge"].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(o.iterations[100].particles["electrons"]["charge"][RecordComponent::SCALAR].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(o.iterations[100].particles["electrons"]["mass"].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(o.iterations[100].particles["electrons"]["mass"][RecordComponent::SCALAR].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(o.iterations[100].particles["electrons"]["momentum"].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(o.iterations[100].particles["electrons"]["momentum"]["x"].parent == getWritable(&o.iterations[100].particles["electrons"]["momentum"]));
        REQUIRE(o.iterations[100].particles["electrons"]["momentum"]["y"].parent == getWritable(&o.iterations[100].particles["electrons"]["momentum"]));
        REQUIRE(o.iterations[100].particles["electrons"]["momentum"]["z"].parent == getWritable(&o.iterations[100].particles["electrons"]["momentum"]));
        REQUIRE(o.iterations[100].particles["electrons"]["position"].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(o.iterations[100].particles["electrons"]["position"]["x"].parent == getWritable(&o.iterations[100].particles["electrons"]["position"]));
        REQUIRE(o.iterations[100].particles["electrons"]["position"]["y"].parent == getWritable(&o.iterations[100].particles["electrons"]["position"]));
        REQUIRE(o.iterations[100].particles["electrons"]["position"]["z"].parent == getWritable(&o.iterations[100].particles["electrons"]["position"]));
        REQUIRE(o.iterations[100].particles["electrons"]["positionOffset"].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(o.iterations[100].particles["electrons"]["positionOffset"]["x"].parent == getWritable(&o.iterations[100].particles["electrons"]["positionOffset"]));
        REQUIRE(o.iterations[100].particles["electrons"]["positionOffset"]["y"].parent == getWritable(&o.iterations[100].particles["electrons"]["positionOffset"]));
        REQUIRE(o.iterations[100].particles["electrons"]["positionOffset"]["z"].parent == getWritable(&o.iterations[100].particles["electrons"]["positionOffset"]));
        REQUIRE(o.iterations[100].particles["electrons"]["weighting"].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE(o.iterations[100].particles["electrons"]["weighting"][RecordComponent::SCALAR].parent == getWritable(&o.iterations[100].particles["electrons"]));
        REQUIRE_THROWS_AS(o.iterations[100].particles["electrons"]["numberOfLegs"], std::out_of_range);
        REQUIRE_THROWS_AS(o.iterations[100].particles["apples"], std::out_of_range);

        int32_t i32 = 32;
        REQUIRE_THROWS(o.setAttribute("setAttributeFail", i32));

    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
#else
    std::cerr << "Invasive tests not enabled. Hierarchy is not visible.\n";
#endif
}

TEST_CASE( "git_hdf5_sample_attribute_test", "[serial][hdf5]" )
{
    try
    {
        Series o = Series("../samples/git-sample/data%T.h5", AccessType::READ_ONLY);

        REQUIRE(o.openPMD() == "1.1.0");
        REQUIRE(o.openPMDextension() == 1);
        REQUIRE(o.basePath() == "/data/%T/");
        REQUIRE(o.meshesPath() == "fields/");
        REQUIRE(o.particlesPath() == "particles/");
        REQUIRE(o.iterationEncoding() == IterationEncoding::fileBased);
        REQUIRE(o.iterationFormat() == "data%T.h5");
        REQUIRE(o.name() == "data%T");

        REQUIRE(o.iterations.size() == 5);
        REQUIRE(o.iterations.count(100) == 1);

        Iteration& iteration_100 = o.iterations[100];
        REQUIRE(iteration_100.time< double >() == 3.2847121452090077e-14);
        REQUIRE(iteration_100.dt< double >() == 3.2847121452090093e-16);
        REQUIRE(iteration_100.timeUnitSI() == 1.0);

        REQUIRE(iteration_100.meshes.size() == 2);
        REQUIRE(iteration_100.meshes.count("E") == 1);
        REQUIRE(iteration_100.meshes.count("rho") == 1);

        std::vector< std::string > al{"x", "y", "z"};
        std::vector< double > gs{8.0000000000000007e-07,
                                 8.0000000000000007e-07,
                                 1.0000000000000001e-07};
        std::vector< double > ggo{-1.0000000000000001e-05,
                                  -1.0000000000000001e-05,
                                  -5.1999999999999993e-06};
        std::array< double, 7 > ud{{1.,  1., -3., -1.,  0.,  0.,  0.}};
        Mesh& E = iteration_100.meshes["E"];
        REQUIRE(E.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(E.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(E.axisLabels() == al);
        REQUIRE(E.gridSpacing< double >() == gs);
        REQUIRE(E.gridGlobalOffset() == ggo);
        REQUIRE(E.gridUnitSI() == 1.0);
        REQUIRE(E.unitDimension() == ud);
        REQUIRE(E.timeOffset< double >() == 0.0);

        REQUIRE(E.size() == 3);
        REQUIRE(E.count("x") == 1);
        REQUIRE(E.count("y") == 1);
        REQUIRE(E.count("z") == 1);

        std::vector< double > p{0.5, 0., 0.};
        Extent e{26, 26, 201};
        MeshRecordComponent& E_x = E["x"];
        REQUIRE(E_x.unitSI() == 1.0);
        REQUIRE(E_x.position< double >() == p);
        REQUIRE(E_x.getDatatype() == Datatype::DOUBLE);
        REQUIRE(E_x.getExtent() == e);
        REQUIRE(E_x.getDimensionality() == 3);

        p = {0., 0.5, 0.};
        MeshRecordComponent& E_y = E["y"];
        REQUIRE(E_y.unitSI() == 1.0);
        REQUIRE(E_y.position< double >() == p);
        REQUIRE(E_y.getDatatype() == Datatype::DOUBLE);
        REQUIRE(E_y.getExtent() == e);
        REQUIRE(E_y.getDimensionality() == 3);

        p = {0., 0., 0.5};
        MeshRecordComponent& E_z = E["z"];
        REQUIRE(E_z.unitSI() == 1.0);
        REQUIRE(E_z.position< double >() == p);
        REQUIRE(E_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(E_z.getExtent() == e);
        REQUIRE(E_z.getDimensionality() == 3);

        gs = {8.0000000000000007e-07,
              8.0000000000000007e-07,
              1.0000000000000001e-07};
        ggo = {-1.0000000000000001e-05,
               -1.0000000000000001e-05,
               -5.1999999999999993e-06};
        ud = {{-3.,  0.,  1.,  1.,  0.,  0.,  0.}};
        Mesh& rho = iteration_100.meshes["rho"];
        REQUIRE(rho.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(rho.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(rho.axisLabels() == al);
        REQUIRE(rho.gridSpacing< double >() == gs);
        REQUIRE(rho.gridGlobalOffset() == ggo);
        REQUIRE(rho.gridUnitSI() == 1.0);
        REQUIRE(rho.unitDimension() == ud);
        REQUIRE(rho.timeOffset< double >() == 0.0);

        REQUIRE(rho.size() == 1);
        REQUIRE(rho.count(MeshRecordComponent::SCALAR) == 1);

        p = {0., 0., 0.};
        e = {26, 26, 201};
        MeshRecordComponent& rho_scalar = rho[MeshRecordComponent::SCALAR];
        REQUIRE(rho_scalar.unitSI() == 1.0);
        REQUIRE(rho_scalar.position< double >() == p);
        REQUIRE(rho_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(rho_scalar.getExtent() == e);
        REQUIRE(rho_scalar.getDimensionality() == 3);

        REQUIRE(iteration_100.particles.size() == 1);
        REQUIRE(iteration_100.particles.count("electrons") == 1);

        ParticleSpecies& electrons = iteration_100.particles["electrons"];

        REQUIRE(electrons.size() == 6);
        REQUIRE(electrons.count("charge") == 1);
        REQUIRE(electrons.count("mass") == 1);
        REQUIRE(electrons.count("momentum") == 1);
        REQUIRE(electrons.count("position") == 1);
        REQUIRE(electrons.count("positionOffset") == 1);
        REQUIRE(electrons.count("weighting") == 1);

        ud = {{0.,  0.,  1.,  1.,  0.,  0.,  0.}};
        Record& charge = electrons["charge"];
        REQUIRE(charge.unitDimension() == ud);
        REQUIRE(charge.timeOffset< double >() == 0.0);

        REQUIRE(charge.size() == 1);
        REQUIRE(charge.count(RecordComponent::SCALAR) == 1);

        e = {85625};
        RecordComponent& charge_scalar = charge[RecordComponent::SCALAR];
        REQUIRE(charge_scalar.unitSI() == 1.0);
        REQUIRE(charge_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(charge_scalar.getDimensionality() == 1);
        REQUIRE(charge_scalar.getExtent() == e);

        ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& mass = electrons["mass"];
        REQUIRE(mass.unitDimension() == ud);
        REQUIRE(mass.timeOffset< double >() == 0.0);

        REQUIRE(mass.size() == 1);
        REQUIRE(mass.count(RecordComponent::SCALAR) == 1);

        RecordComponent& mass_scalar = mass[RecordComponent::SCALAR];
        REQUIRE(mass_scalar.unitSI() == 1.0);
        REQUIRE(mass_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(mass_scalar.getDimensionality() == 1);
        REQUIRE(mass_scalar.getExtent() == e);

        ud = {{1.,  1., -1.,  0.,  0.,  0.,  0.}};
        Record& momentum = electrons["momentum"];
        REQUIRE(momentum.unitDimension() == ud);
        REQUIRE(momentum.timeOffset< double >() == 0.0);

        REQUIRE(momentum.size() == 3);
        REQUIRE(momentum.count("x") == 1);
        REQUIRE(momentum.count("y") == 1);
        REQUIRE(momentum.count("z") == 1);

        RecordComponent& momentum_x = momentum["x"];
        REQUIRE(momentum_x.unitSI() == 1.0);
        REQUIRE(momentum_x.getDatatype() == Datatype::DOUBLE);
        REQUIRE(momentum_x.getDimensionality() == 1);
        REQUIRE(momentum_x.getExtent() == e);

        RecordComponent& momentum_y = momentum["y"];
        REQUIRE(momentum_y.unitSI() == 1.0);
        REQUIRE(momentum_y.getDatatype() == Datatype::DOUBLE);
        REQUIRE(momentum_y.getDimensionality() == 1);
        REQUIRE(momentum_y.getExtent() == e);

        RecordComponent& momentum_z = momentum["z"];
        REQUIRE(momentum_z.unitSI() == 1.0);
        REQUIRE(momentum_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(momentum_z.getDimensionality() == 1);
        REQUIRE(momentum_z.getExtent() == e);

        ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& position = electrons["position"];
        REQUIRE(position.unitDimension() == ud);
        REQUIRE(position.timeOffset< double >() == 0.0);

        REQUIRE(position.size() == 3);
        REQUIRE(position.count("x") == 1);
        REQUIRE(position.count("y") == 1);
        REQUIRE(position.count("z") == 1);

        RecordComponent& position_x = position["x"];
        REQUIRE(position_x.unitSI() == 1.0);
        REQUIRE(position_x.getDatatype() == Datatype::DOUBLE);
        REQUIRE(position_x.getDimensionality() == 1);
        REQUIRE(position_x.getExtent() == e);

        RecordComponent& position_y = position["y"];
        REQUIRE(position_y.unitSI() == 1.0);
        REQUIRE(position_y.getDatatype() == Datatype::DOUBLE);
        REQUIRE(position_y.getDimensionality() == 1);
        REQUIRE(position_y.getExtent() == e);

        RecordComponent& position_z = position["z"];
        REQUIRE(position_z.unitSI() == 1.0);
        REQUIRE(position_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(position_z.getDimensionality() == 1);
        REQUIRE(position_z.getExtent() == e);

        Record& positionOffset = electrons["positionOffset"];
        REQUIRE(positionOffset.unitDimension() == ud);
        REQUIRE(positionOffset.timeOffset< double >() == 0.0);

        REQUIRE(positionOffset.size() == 3);
        REQUIRE(positionOffset.count("x") == 1);
        REQUIRE(positionOffset.count("y") == 1);
        REQUIRE(positionOffset.count("z") == 1);

        RecordComponent& positionOffset_x = positionOffset["x"];
        REQUIRE(positionOffset_x.unitSI() == 1.0);
        REQUIRE(positionOffset_x.getDatatype() == Datatype::DOUBLE);
        REQUIRE(positionOffset_x.getDimensionality() == 1);
        REQUIRE(positionOffset_x.getExtent() == e);

        RecordComponent& positionOffset_y = positionOffset["y"];
        REQUIRE(positionOffset_y.unitSI() == 1.0);
        REQUIRE(positionOffset_y.getDatatype() == Datatype::DOUBLE);
        REQUIRE(positionOffset_y.getDimensionality() == 1);
        REQUIRE(positionOffset_y.getExtent() == e);

        RecordComponent& positionOffset_z = positionOffset["z"];
        REQUIRE(positionOffset_z.unitSI() == 1.0);
        REQUIRE(positionOffset_z.getDatatype() == Datatype::DOUBLE);
        REQUIRE(positionOffset_z.getDimensionality() == 1);
        REQUIRE(positionOffset_z.getExtent() == e);

        ud = {{0.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& weighting = electrons["weighting"];
        REQUIRE(weighting.unitDimension() == ud);
        REQUIRE(weighting.timeOffset< double >() == 0.0);

        REQUIRE(weighting.size() == 1);
        REQUIRE(weighting.count(RecordComponent::SCALAR) == 1);

        RecordComponent& weighting_scalar = weighting[RecordComponent::SCALAR];
        REQUIRE(weighting_scalar.unitSI() == 1.0);
        REQUIRE(weighting_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(weighting_scalar.getDimensionality() == 1);
        REQUIRE(weighting_scalar.getExtent() == e);
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

TEST_CASE( "git_hdf5_sample_content_test", "[serial][hdf5]" )
{
    try
    {
        Series o = Series("../samples/git-sample/data%T.h5", AccessType::READ_ONLY);

        {
            double actual[3][3][3] = {{{-1.9080703683727052e-09, -1.5632650729457964e-10, 1.1497536256399599e-09},
                                       {-1.9979540244463578e-09, -2.5512036927466397e-10, 1.0402234629225404e-09},
                                       {-1.7353589676361025e-09, -8.0899198451334087e-10, -1.6443779671249104e-10}},

                                      {{-2.0029988778702545e-09, -1.9543477947081556e-10, 1.0916454407094989e-09},
                                       {-2.3890367462087170e-09, -4.7158010829662089e-10, 9.0026075483251589e-10},
                                       {-1.9033881137886510e-09, -7.5192119197708962e-10, 5.0038861942880430e-10}},

                                      {{-1.3271805876513554e-09, -5.9243276950837753e-10, -2.2445734160214670e-10},
                                       {-7.4578609954301101e-10, -1.1995737736469891e-10, 2.5611823772919706e-10},
                                       {-9.4806251738077663e-10, -1.5472800818372434e-10, -3.6461900165818406e-10}}};
            MeshRecordComponent& rho = o.iterations[100].meshes["rho"][MeshRecordComponent::SCALAR];
            Offset offset{20, 20, 190};
            Extent extent{3, 3, 3};
            auto data = rho.loadChunk<double>(offset, extent);
            o.flush();
            double* raw_ptr = data.get();

            for( int i = 0; i < 3; ++i )
                for( int j = 0; j < 3; ++j )
                    for( int k = 0; k < 3; ++k )
                        REQUIRE(raw_ptr[((i*3) + j)*3 + k] == actual[i][j][k]);
        }

        {
            double constant_value = 9.1093829099999999e-31;
            RecordComponent& electrons_mass = o.iterations[100].particles["electrons"]["mass"][RecordComponent::SCALAR];
            Offset offset{15};
            Extent extent{3};
            auto data = electrons_mass.loadChunk<double>(offset, extent);
            o.flush();
            double* raw_ptr = data.get();

            for( int i = 0; i < 3; ++i )
                REQUIRE(raw_ptr[i] == constant_value);
        }
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

TEST_CASE( "git_hdf5_sample_fileBased_read_test", "[serial][hdf5]" )
{
    try
    {
        Series o = Series("../samples/git-sample/data%T.h5", AccessType::READ_ONLY);

        REQUIRE(o.iterations.size() == 5);
        REQUIRE(o.iterations.count(100) == 1);
        REQUIRE(o.iterations.count(200) == 1);
        REQUIRE(o.iterations.count(300) == 1);
        REQUIRE(o.iterations.count(400) == 1);
        REQUIRE(o.iterations.count(500) == 1);
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

TEST_CASE( "hzdr_hdf5_sample_content_test", "[serial][hdf5]" )
{
    // since this file might not be publicly available, gracefully handle errors
    try
    {
        /* development/huebl/lwfa-openPMD-062-smallLWFA-h5 */
        Series o = Series("../samples/hzdr-sample/h5/simData_%T.h5", AccessType::READ_ONLY);

        REQUIRE(o.openPMD() == "1.0.0");
        REQUIRE(o.openPMDextension() == 1);
        REQUIRE(o.basePath() == "/data/%T/");
        REQUIRE(o.meshesPath() == "fields/");
        REQUIRE(o.particlesPath() == "particles/");
        REQUIRE(o.author() == "Axel Huebl <a.huebl@hzdr.de>");
        REQUIRE(o.software() == "PIConGPU");
        REQUIRE(o.softwareVersion() == "0.2.0");
        REQUIRE(o.date() == "2016-11-04 00:59:14 +0100");
        REQUIRE(o.iterationEncoding() == IterationEncoding::fileBased);
        REQUIRE(o.iterationFormat() == "h5/simData_%T.h5");
        REQUIRE(o.name() == "simData_%T");

        REQUIRE(o.iterations.size() >= 1);
        REQUIRE(o.iterations.count(0) == 1);

        Iteration& i = o.iterations[0];
        REQUIRE(i.time< float >() == static_cast< float >(0.0f));
        REQUIRE(i.dt< float >() == static_cast< float >(1.0f));
        REQUIRE(i.timeUnitSI() == 1.3899999999999999e-16);

        REQUIRE(i.meshes.size() == 4);
        REQUIRE(i.meshes.count("B") == 1);
        REQUIRE(i.meshes.count("E") == 1);
        REQUIRE(i.meshes.count("e_chargeDensity") == 1);
        REQUIRE(i.meshes.count("e_energyDensity") == 1);

        std::vector< std::string > al{"z", "y", "x"};
        std::vector< float > gs{static_cast< float >(6.2393283843994141f),
                                static_cast< float >(1.0630855560302734f),
                                static_cast< float >(6.2393283843994141f)};
        std::vector< double > ggo{0., 0., 0.};
        std::array< double, 7 > ud{{0.,  1., -2., -1.,  0.,  0.,  0.}};
        Mesh& B = i.meshes["B"];
        REQUIRE(B.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(B.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(B.axisLabels() == al);
        REQUIRE(B.gridSpacing< float >() == gs);
        REQUIRE(B.gridGlobalOffset() == ggo);
        REQUIRE(B.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(B.unitDimension() == ud);
        REQUIRE(B.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(B.size() == 3);
        REQUIRE(B.count("x") == 1);
        REQUIRE(B.count("y") == 1);
        REQUIRE(B.count("z") == 1);

        std::vector< float > p{static_cast< float >(0.0f),
                               static_cast< float >(0.5f),
                               static_cast< float >(0.5f)};
        Extent e{80, 384, 80};
        MeshRecordComponent& B_x = B["x"];
        REQUIRE(B_x.unitSI() == 40903.822240601701);
        REQUIRE(B_x.position< float >() == p);
        REQUIRE(B_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_x.getExtent() == e);
        REQUIRE(B_x.getDimensionality() == 3);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.0f),
             static_cast< float >(0.5f)};
        MeshRecordComponent& B_y = B["y"];
        REQUIRE(B_y.unitSI() == 40903.822240601701);
        REQUIRE(B_y.position< float >() == p);
        REQUIRE(B_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_y.getExtent() == e);
        REQUIRE(B_y.getDimensionality() == 3);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.5f),
             static_cast< float >(0.0f)};
        MeshRecordComponent& B_z = B["z"];
        REQUIRE(B_z.unitSI() == 40903.822240601701);
        REQUIRE(B_z.position< float >() == p);
        REQUIRE(B_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_z.getExtent() == e);
        REQUIRE(B_z.getDimensionality() == 3);

        ud = {{1.,  1., -3., -1.,  0.,  0.,  0.}};
        Mesh& E = i.meshes["E"];
        REQUIRE(E.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(E.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(E.axisLabels() == al);
        REQUIRE(E.gridSpacing< float >() == gs);
        REQUIRE(E.gridGlobalOffset() == ggo);
        REQUIRE(E.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(E.unitDimension() == ud);
        REQUIRE(E.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(E.size() == 3);
        REQUIRE(E.count("x") == 1);
        REQUIRE(E.count("y") == 1);
        REQUIRE(E.count("z") == 1);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.0f),
             static_cast< float >(0.0f)};
        e = {80, 384, 80};
        MeshRecordComponent& E_x = E["x"];
        REQUIRE(E_x.unitSI() == 12262657411105.049);
        REQUIRE(E_x.position< float >() == p);
        REQUIRE(E_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_x.getExtent() == e);
        REQUIRE(E_x.getDimensionality() == 3);

        p = {static_cast< float >(0.0f),
             static_cast< float >(0.5f),
             static_cast< float >(0.0f)};
        MeshRecordComponent& E_y = E["y"];
        REQUIRE(E_y.unitSI() == 12262657411105.049);
        REQUIRE(E_y.position< float >() == p);
        REQUIRE(E_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_y.getExtent() == e);
        REQUIRE(E_y.getDimensionality() == 3);

        p = {static_cast< float >(0.0f),
             static_cast< float >(0.0f),
             static_cast< float >(0.5f)};
        MeshRecordComponent& E_z = E["z"];
        REQUIRE(E_z.unitSI() == 12262657411105.049);
        REQUIRE(E_z.position< float >() == p);
        REQUIRE(E_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_z.getExtent() == e);
        REQUIRE(E_z.getDimensionality() == 3);

        ud = {{-3.,  0.,  1.,  1.,  0.,  0.,  0.}};
        Mesh& e_chargeDensity = i.meshes["e_chargeDensity"];
        REQUIRE(e_chargeDensity.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(e_chargeDensity.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(e_chargeDensity.axisLabels() == al);
        REQUIRE(e_chargeDensity.gridSpacing< float >() == gs);
        REQUIRE(e_chargeDensity.gridGlobalOffset() == ggo);
        REQUIRE(e_chargeDensity.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(e_chargeDensity.unitDimension() == ud);
        REQUIRE(e_chargeDensity.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(e_chargeDensity.size() == 1);
        REQUIRE(e_chargeDensity.count(MeshRecordComponent::SCALAR) == 1);

        p = {static_cast< float >(0.f),
             static_cast< float >(0.f),
             static_cast< float >(0.f)};
        MeshRecordComponent& e_chargeDensity_scalar = e_chargeDensity[MeshRecordComponent::SCALAR];
        REQUIRE(e_chargeDensity_scalar.unitSI() == 66306201.002331272);
        REQUIRE(e_chargeDensity_scalar.position< float >() == p);
        REQUIRE(e_chargeDensity_scalar.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_chargeDensity_scalar.getExtent() == e);
        REQUIRE(e_chargeDensity_scalar.getDimensionality() == 3);

        ud = {{-1.,  1., -2.,  0.,  0.,  0.,  0.}};
        Mesh& e_energyDensity = i.meshes["e_energyDensity"];
        REQUIRE(e_energyDensity.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(e_energyDensity.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(e_energyDensity.axisLabels() == al);
        REQUIRE(e_energyDensity.gridSpacing< float >() == gs);
        REQUIRE(e_energyDensity.gridGlobalOffset() == ggo);
        REQUIRE(e_energyDensity.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(e_energyDensity.unitDimension() == ud);
        REQUIRE(e_energyDensity.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(e_energyDensity.size() == 1);
        REQUIRE(e_energyDensity.count(MeshRecordComponent::SCALAR) == 1);

        MeshRecordComponent& e_energyDensity_scalar = e_energyDensity[MeshRecordComponent::SCALAR];
        REQUIRE(e_energyDensity_scalar.unitSI() == 1.0146696675429705e+18);
        REQUIRE(e_energyDensity_scalar.position< float >() == p);
        REQUIRE(e_energyDensity_scalar.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_energyDensity_scalar.getExtent() == e);
        REQUIRE(e_energyDensity_scalar.getDimensionality() == 3);

        REQUIRE(i.particles.size() == 1);
        REQUIRE(i.particles.count("e") == 1);

        ParticleSpecies& species_e = i.particles["e"];

        REQUIRE(species_e.size() == 6);
        REQUIRE(species_e.count("charge") == 1);
        REQUIRE(species_e.count("mass") == 1);
        REQUIRE(species_e.count("momentum") == 1);
        REQUIRE(species_e.count("particlePatches") == 0);
        REQUIRE(species_e.count("position") == 1);
        REQUIRE(species_e.count("positionOffset") == 1);
        REQUIRE(species_e.count("weighting") == 1);

        ud = {{0.,  0.,  1.,  1.,  0.,  0.,  0.}};
        Record& e_charge = species_e["charge"];
        REQUIRE(e_charge.unitDimension() == ud);
        REQUIRE(e_charge.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(e_charge.size() == 1);
        REQUIRE(e_charge.count(RecordComponent::SCALAR) == 1);

        e = {2150400};
        RecordComponent& e_charge_scalar = e_charge[RecordComponent::SCALAR];
        REQUIRE(e_charge_scalar.unitSI() == 4.7980045488500004e-15);
        REQUIRE(e_charge_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(e_charge_scalar.getExtent() == e);
        REQUIRE(e_charge_scalar.getDimensionality() == 1);

        ud = {{0.,  1.,  0.,  0.,  0.,  0.,  0.}};
        Record& e_mass = species_e["mass"];
        REQUIRE(e_mass.unitDimension() == ud);
        REQUIRE(e_mass.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(e_mass.size() == 1);
        REQUIRE(e_mass.count(RecordComponent::SCALAR) == 1);

        RecordComponent& e_mass_scalar = e_mass[RecordComponent::SCALAR];
        REQUIRE(e_mass_scalar.unitSI() == 2.7279684799430467e-26);
        REQUIRE(e_mass_scalar.getDatatype() == Datatype::DOUBLE);
        REQUIRE(e_mass_scalar.getExtent() == e);
        REQUIRE(e_mass_scalar.getDimensionality() == 1);

        ud = {{1.,  1., -1.,  0.,  0.,  0.,  0.}};
        Record& e_momentum = species_e["momentum"];
        REQUIRE(e_momentum.unitDimension() == ud);
        REQUIRE(e_momentum.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(e_momentum.size() == 3);
        REQUIRE(e_momentum.count("x") == 1);
        REQUIRE(e_momentum.count("y") == 1);
        REQUIRE(e_momentum.count("z") == 1);

        RecordComponent& e_momentum_x = e_momentum["x"];
        REQUIRE(e_momentum_x.unitSI() == 8.1782437594864961e-18);
        REQUIRE(e_momentum_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_momentum_x.getExtent() == e);
        REQUIRE(e_momentum_x.getDimensionality() == 1);

        RecordComponent& e_momentum_y = e_momentum["y"];
        REQUIRE(e_momentum_y.unitSI() == 8.1782437594864961e-18);
        REQUIRE(e_momentum_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_momentum_y.getExtent() == e);
        REQUIRE(e_momentum_y.getDimensionality() == 1);

        RecordComponent& e_momentum_z = e_momentum["z"];
        REQUIRE(e_momentum_z.unitSI() == 8.1782437594864961e-18);
        REQUIRE(e_momentum_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_momentum_z.getExtent() == e);
        REQUIRE(e_momentum_z.getDimensionality() == 1);

        ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& e_position = species_e["position"];
        REQUIRE(e_position.unitDimension() == ud);
        REQUIRE(e_position.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(e_position.size() == 3);
        REQUIRE(e_position.count("x") == 1);
        REQUIRE(e_position.count("y") == 1);
        REQUIRE(e_position.count("z") == 1);

        RecordComponent& e_position_x = e_position["x"];
        REQUIRE(e_position_x.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_position_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_position_x.getExtent() == e);
        REQUIRE(e_position_x.getDimensionality() == 1);

        RecordComponent& e_position_y = e_position["y"];
        REQUIRE(e_position_y.unitSI() == 4.4299999435019118e-08);
        REQUIRE(e_position_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_position_y.getExtent() == e);
        REQUIRE(e_position_y.getDimensionality() == 1);

        RecordComponent& e_position_z = e_position["z"];
        REQUIRE(e_position_z.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_position_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_position_z.getExtent() == e);
        REQUIRE(e_position_z.getDimensionality() == 1);

        ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& e_positionOffset = species_e["positionOffset"];
        REQUIRE(e_positionOffset.unitDimension() == ud);
        REQUIRE(e_positionOffset.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(e_positionOffset.size() == 3);
        REQUIRE(e_positionOffset.count("x") == 1);
        REQUIRE(e_positionOffset.count("y") == 1);
        REQUIRE(e_positionOffset.count("z") == 1);

        RecordComponent& e_positionOffset_x = e_positionOffset["x"];
        REQUIRE(e_positionOffset_x.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_positionOffset_x.getDatatype() == Datatype::INT32);
        REQUIRE(e_positionOffset_x.getExtent() == e);
        REQUIRE(e_positionOffset_x.getDimensionality() == 1);

        RecordComponent& e_positionOffset_y = e_positionOffset["y"];
        REQUIRE(e_positionOffset_y.unitSI() == 4.4299999435019118e-08);
        REQUIRE(e_positionOffset_y.getDatatype() == Datatype::INT32);
        REQUIRE(e_positionOffset_y.getExtent() == e);
        REQUIRE(e_positionOffset_y.getDimensionality() == 1);

        RecordComponent& e_positionOffset_z = e_positionOffset["z"];
        REQUIRE(e_positionOffset_z.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_positionOffset_z.getDatatype() == Datatype::INT32);
        REQUIRE(e_positionOffset_z.getExtent() == e);
        REQUIRE(e_positionOffset_z.getDimensionality() == 1);

        ud = {{0.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& e_weighting = species_e["weighting"];
        REQUIRE(e_weighting.unitDimension() == ud);
        REQUIRE(e_weighting.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(e_weighting.size() == 1);
        REQUIRE(e_weighting.count(RecordComponent::SCALAR) == 1);

        RecordComponent& e_weighting_scalar = e_weighting[RecordComponent::SCALAR];
        REQUIRE(e_weighting_scalar.unitSI() == 1.0);
        REQUIRE(e_weighting_scalar.getDatatype() == Datatype::FLOAT);
        REQUIRE(e_weighting_scalar.getExtent() == e);
        REQUIRE(e_weighting_scalar.getDimensionality() == 1);

        ParticlePatches& e_patches = species_e.particlePatches;
        REQUIRE(e_patches.size() == 4); /* extent, numParticles, numParticlesOffset, offset */
        REQUIRE(e_patches.count("extent") == 1);
        REQUIRE(e_patches.count("numParticles") == 1);
        REQUIRE(e_patches.count("numParticlesOffset") == 1);
        REQUIRE(e_patches.count("offset") == 1);
        REQUIRE(e_patches.numPatches() == 4);

        ud = {{1., 0.,  0.,  0.,  0.,  0.,  0.}};
        PatchRecord& e_extent = e_patches["extent"];
        REQUIRE(e_extent.unitDimension() == ud);

        REQUIRE(e_extent.size() == 3);
        REQUIRE(e_extent.count("x") == 1);
        REQUIRE(e_extent.count("y") == 1);
        REQUIRE(e_extent.count("z") == 1);

        PatchRecordComponent& e_extent_x = e_extent["x"];
        REQUIRE(e_extent_x.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_extent_x.getDatatype() == Datatype::UINT64);

        PatchRecordComponent& e_extent_y = e_extent["y"];
        REQUIRE(e_extent_y.unitSI() == 4.429999943501912e-08);
        REQUIRE(e_extent_y.getDatatype() == Datatype::UINT64);

        PatchRecordComponent& e_extent_z = e_extent["z"];
        REQUIRE(e_extent_z.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_extent_z.getDatatype() == Datatype::UINT64);

        std::shared_ptr< uint64_t > data;
        e_extent_z.load(0, data);
        o.flush();
        REQUIRE(*data == static_cast< uint64_t >(80));

        PatchRecord& e_numParticles = e_patches["numParticles"];
        REQUIRE(e_numParticles.size() == 1);
        REQUIRE(e_numParticles.count(RecordComponent::SCALAR) == 1);

        PatchRecordComponent& e_numParticles_scalar = e_numParticles[RecordComponent::SCALAR];
        REQUIRE(e_numParticles_scalar.getDatatype() == Datatype::UINT64);

        e_numParticles_scalar.load(0, data);
        o.flush();
        REQUIRE(*data == static_cast< uint64_t >(512000));

        PatchRecord& e_numParticlesOffset = e_patches["numParticlesOffset"];
        REQUIRE(e_numParticlesOffset.size() == 1);
        REQUIRE(e_numParticlesOffset.count(RecordComponent::SCALAR) == 1);

        PatchRecordComponent& e_numParticlesOffset_scalar = e_numParticlesOffset[RecordComponent::SCALAR];
        REQUIRE(e_numParticlesOffset_scalar.getDatatype() == Datatype::UINT64);

        PatchRecord& e_offset = e_patches["offset"];
        REQUIRE(e_offset.unitDimension() == ud);

        REQUIRE(e_offset.size() == 3);
        REQUIRE(e_offset.count("x") == 1);
        REQUIRE(e_offset.count("y") == 1);
        REQUIRE(e_offset.count("z") == 1);

        PatchRecordComponent& e_offset_x = e_offset["x"];
        REQUIRE(e_offset_x.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_offset_x.getDatatype() == Datatype::UINT64);

        PatchRecordComponent& e_offset_y = e_offset["y"];
        REQUIRE(e_offset_y.unitSI() == 4.429999943501912e-08);
        REQUIRE(e_offset_y.getDatatype() == Datatype::UINT64);

        PatchRecordComponent& e_offset_z = e_offset["z"];
        REQUIRE(e_offset_z.unitSI() == 2.599999993753294e-07);
        REQUIRE(e_offset_z.getDatatype() == Datatype::UINT64);
    } catch (no_such_file_error& e)
    {
        std::cerr << "HZDR sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

TEST_CASE( "hdf5_dtype_test", "[serial][hdf5]" )
{
    {
        Series s = Series("../samples/dtype_test.h5", AccessType::CREATE);

        char c = 'c';
        s.setAttribute("char", c);
        unsigned char uc = 'u';
        s.setAttribute("uchar", uc);
        int16_t i16 = 16;
        s.setAttribute("int16", i16);
        int32_t i32 = 32;
        s.setAttribute("int32", i32);
        int64_t i64 = 64;
        s.setAttribute("int64", i64);
        uint16_t u16 = 16u;
        s.setAttribute("uint16", u16);
        uint32_t u32 = 32u;
        s.setAttribute("uint32", u32);
        uint64_t u64 = 64u;
        s.setAttribute("uint64", u64);
        float f = 16.e10f;
        s.setAttribute("float", f);
        double d = 1.e64;
        s.setAttribute("double", d);
        long double l = 1.e80L;
        s.setAttribute("longdouble", l);
        std::string str = "string";
        s.setAttribute("string", str);
        s.setAttribute("vecChar", std::vector< char >({'c', 'h', 'a', 'r'}));
        s.setAttribute("vecInt16", std::vector< int16_t >({32766, 32767}));
        s.setAttribute("vecInt32", std::vector< int32_t >({2147483646, 2147483647}));
        s.setAttribute("vecInt64", std::vector< int64_t >({9223372036854775806, 9223372036854775807}));
        s.setAttribute("vecUchar", std::vector< char >({'u', 'c', 'h', 'a', 'r'}));
        s.setAttribute("vecUint16", std::vector< uint16_t >({65534u, 65535u}));
        s.setAttribute("vecUint32", std::vector< uint32_t >({4294967294u, 4294967295u}));
        s.setAttribute("vecUint64", std::vector< uint64_t >({18446744073709551614u, 18446744073709551615u}));
        s.setAttribute("vecFloat", std::vector< float >({0.f, 3.40282e+38f}));
        s.setAttribute("vecDouble", std::vector< double >({0., 1.79769e+308}));
        s.setAttribute("vecLongdouble", std::vector< long double >({0.L, std::numeric_limits<long double>::max()}));
        s.setAttribute("vecString", std::vector< std::string >({"vector", "of", "strings"}));
        s.setAttribute("bool", true);
    }

    Series s = Series("../samples/dtype_test.h5", AccessType::READ_ONLY);

    REQUIRE(s.getAttribute("char").get< char >() == 'c');
    REQUIRE(s.getAttribute("uchar").get< unsigned char >() == 'u');
    REQUIRE(s.getAttribute("int16").get< int16_t >() == 16);
    REQUIRE(s.getAttribute("int32").get< int32_t >() == 32);
    REQUIRE(s.getAttribute("int64").get< int64_t >() == 64);
    REQUIRE(s.getAttribute("uint16").get< uint16_t >() == 16u);
    REQUIRE(s.getAttribute("uint32").get< uint32_t >() == 32u);
    REQUIRE(s.getAttribute("uint64").get< uint64_t >() == 64u);
    REQUIRE(s.getAttribute("float").get< float >() == 16.e10f);
    REQUIRE(s.getAttribute("double").get< double >() == 1.e64);
#if !defined(_MSC_VER)
    REQUIRE(s.getAttribute("longdouble").get< long double >() == 1.e80L);
#endif
    REQUIRE(s.getAttribute("string").get< std::string >() == "string");
    REQUIRE(s.getAttribute("vecChar").get< std::vector< char > >() == std::vector< char >({'c', 'h', 'a', 'r'}));
    REQUIRE(s.getAttribute("vecInt16").get< std::vector< int16_t > >() == std::vector< int16_t >({32766, 32767}));
    REQUIRE(s.getAttribute("vecInt32").get< std::vector< int32_t > >() == std::vector< int32_t >({2147483646, 2147483647}));
    REQUIRE(s.getAttribute("vecInt64").get< std::vector< int64_t > >() == std::vector< int64_t >({9223372036854775806, 9223372036854775807}));
    REQUIRE(s.getAttribute("vecUchar").get< std::vector< char > >() == std::vector< char >({'u', 'c', 'h', 'a', 'r'}));
    REQUIRE(s.getAttribute("vecUint16").get< std::vector< uint16_t > >() == std::vector< uint16_t >({65534u, 65535u}));
    REQUIRE(s.getAttribute("vecUint32").get< std::vector< uint32_t > >() == std::vector< uint32_t >({4294967294u, 4294967295u}));
    REQUIRE(s.getAttribute("vecUint64").get< std::vector< uint64_t > >() == std::vector< uint64_t >({18446744073709551614u, 18446744073709551615u}));
    REQUIRE(s.getAttribute("vecFloat").get< std::vector< float > >() == std::vector< float >({0.f, 3.40282e+38f}));
    REQUIRE(s.getAttribute("vecDouble").get< std::vector< double > >() == std::vector< double >({0., 1.79769e+308}));
#if !defined(_MSC_VER)
    REQUIRE(s.getAttribute("vecLongdouble").get< std::vector< long double > >() == std::vector< long double >({0.L, std::numeric_limits<long double>::max()}));
#endif
    REQUIRE(s.getAttribute("vecString").get< std::vector< std::string > >() == std::vector< std::string >({"vector", "of", "strings"}));
    REQUIRE(s.getAttribute("bool").get< bool >() == true);
}

TEST_CASE( "hdf5_write_test", "[serial][hdf5]" )
{
    Series o = Series("../samples/serial_write.h5", AccessType::CREATE);

    o.setAuthor("Serial HDF5");
    ParticleSpecies& e = o.iterations[1].particles["e"];

    std::vector< double > position_global(4);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::vector< double > position_local = {0.};
    e["position"]["x"].resetDataset(Dataset(determineDatatype<double>(), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        position_local.at(0) = position_global[i];
        e["position"]["x"].storeChunk({i}, {1}, shareRaw(position_local));
        o.flush();
    }

    std::vector< uint64_t > positionOffset_global(4);
    uint64_t posOff{0};
    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::array< uint64_t, 1 > positionOffset_local = {{ 0u }};
    e["positionOffset"]["x"].resetDataset(Dataset(determineDatatype<uint64_t>(), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        positionOffset_local[0] = positionOffset_global[i];
        e["positionOffset"]["x"].storeChunk({i}, {1}, shareRaw(positionOffset_local));
        o.flush();
    }

    o.flush();

    //TODO close file, read back, verify
}

TEST_CASE( "hdf5_fileBased_write_test", "[serial][hdf5]" )
{
    Series o = Series("../samples/serial_fileBased_write%T.h5", AccessType::CREATE);

    ParticleSpecies& e_1 = o.iterations[1].particles["e"];

    std::vector< double > position_global(4);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local_1(new double);
    e_1["position"]["x"].resetDataset(Dataset(determineDatatype(position_local_1), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local_1 = position_global[i];
        e_1["position"]["x"].storeChunk({i}, {1}, position_local_1);
        o.flush();
    }

    std::vector< uint64_t > positionOffset_global(4);
    uint64_t posOff{0};
    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_1(new uint64_t);
    e_1["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_1), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_1 = positionOffset_global[i];
        e_1["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_1);
        o.flush();
    }

    ParticleSpecies& e_2 = o.iterations[2].particles["e"];

    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    e_2["position"]["x"].resetDataset(Dataset(determineDatatype<double>(), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        double const position_local_2 = position_global.at(i);
        e_2["position"]["x"].storeChunk({i}, {1}, shareRaw(&position_local_2));
        o.flush();
    }

    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_2(new uint64_t);
    e_2["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_2), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_2 = positionOffset_global[i];
        e_2["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_2);
        o.flush();
    }

    o.flush();

    ParticleSpecies& e_3 = o.iterations[3].particles["e"];

    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local_3(new double);
    e_3["position"]["x"].resetDataset(Dataset(determineDatatype(position_local_3), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local_3 = position_global[i];
        e_3["position"]["x"].storeChunk({i}, {1}, position_local_3);
        o.flush();
    }

    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_3(new uint64_t);
    e_3["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_3), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_3 = positionOffset_global[i];
        e_3["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_3);
        o.flush();
    }

    o.flush();

    //TODO close file, read back, verify
}

TEST_CASE( "hdf5_bool_test", "[serial][hdf5]" )
{
    {
        Series o = Series("../samples/serial_bool.h5", AccessType::CREATE);

        o.setAttribute("Bool attribute (true)", true);
        o.setAttribute("Bool attribute (false)", false);
    }
    {
        Series o = Series("../samples/serial_bool.h5", AccessType::READ_ONLY);

        auto attrs = o.attributes();
        REQUIRE(std::count(attrs.begin(), attrs.end(), "Bool attribute (true)") == 1);
        REQUIRE(std::count(attrs.begin(), attrs.end(), "Bool attribute (false)") == 1);
        REQUIRE(o.getAttribute("Bool attribute (true)").get< bool >() == true);
        REQUIRE(o.getAttribute("Bool attribute (false)").get< bool >() == false);
    }
}

TEST_CASE( "hdf5_patch_test", "[serial][hdf5]" )
{
    Series o = Series("../samples/serial_patch.h5", AccessType::CREATE);

    o.iterations[1].particles["e"].particlePatches["offset"]["x"].setUnitSI(42);
}

TEST_CASE( "hdf5_deletion_test", "[serial][hdf5]" )
{
    Series o = Series("../samples/serial_deletion.h5", AccessType::CREATE);


    o.setAttribute("removed",
                   "this attribute will be removed after being written to disk");
    o.flush();

    o.deleteAttribute("removed");
    o.flush();

    ParticleSpecies& e = o.iterations[1].particles["e"];
    e.erase("deletion");
    o.flush();

    e["deletion_scalar"][RecordComponent::SCALAR].resetDataset(Dataset(Datatype::DOUBLE, {1}));
    o.flush();

    e["deletion_scalar"].erase(RecordComponent::SCALAR);
    o.flush();

    e.erase("deletion_scalar");
    o.flush();

    double value = 0.;
    e["deletion_scalar_constant"][RecordComponent::SCALAR].resetDataset(Dataset(Datatype::DOUBLE, {1}));
    e["deletion_scalar_constant"][RecordComponent::SCALAR].makeConstant(value);
    o.flush();

    e["deletion_scalar_constant"].erase(RecordComponent::SCALAR);
    o.flush();

    e.erase("deletion_scalar_constant");
    o.flush();
}

TEST_CASE( "hdf5_110_optional_paths", "[serial][hdf5]" )
{
    try
    {
        {
            Series s = Series("../samples/issue-sample/no_fields/data%T.h5", AccessType::READ_ONLY);
            auto attrs = s.attributes();
            REQUIRE(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
            REQUIRE(std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
            REQUIRE(s.iterations[400].meshes.empty());
            REQUIRE(s.iterations[400].particles.size() == 1);
        }

        {
            Series s = Series("../samples/issue-sample/no_particles/data%T.h5", AccessType::READ_ONLY);
            auto attrs = s.attributes();
            REQUIRE(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
            REQUIRE(std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
            REQUIRE(s.iterations[400].meshes.size() == 2);
            REQUIRE(s.iterations[400].particles.empty());
        }
    } catch (no_such_file_error& e)
    {
        std::cerr << "issue sample not accessible. (" << e.what() << ")\n";
    }

    {
        Series s = Series("../samples/no_meshes_1.1.0_compliant.h5", AccessType::CREATE);
        s.iterations[1].particles["foo"];
    }

    {
        Series s = Series("../samples/no_particles_1.1.0_compliant.h5", AccessType::CREATE);
        s.iterations[1].meshes["foo"];
    }

    {
        Series s = Series("../samples/no_meshes_1.1.0_compliant.h5", AccessType::READ_ONLY);
        auto attrs = s.attributes();
        REQUIRE(std::count(attrs.begin(), attrs.end(), "meshesPath") == 0);
        REQUIRE(std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
        REQUIRE(s.iterations[1].meshes.empty());
        REQUIRE(s.iterations[1].particles.size() == 1);
    }

    {
        Series s = Series("../samples/no_particles_1.1.0_compliant.h5", AccessType::READ_ONLY);
        auto attrs = s.attributes();
        REQUIRE(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
        REQUIRE(std::count(attrs.begin(), attrs.end(), "particlesPath") == 0);
        REQUIRE(s.iterations[1].meshes.size() == 1);
        REQUIRE(s.iterations[1].particles.empty());
    }
}
#else
TEST_CASE( "no_serial_hdf5", "[serial][hdf5]" )
{
    REQUIRE(true);
}
#endif
#if openPMD_HAVE_ADIOS1
TEST_CASE( "adios1_dtype_test", "[serial][adios1]" )
{
    {
        Series s = Series("../samples/dtype_test.bp", AccessType::CREATE);

        char c = 'c';
        s.setAttribute("char", c);
        unsigned char uc = 'u';
        s.setAttribute("uchar", uc);
        int16_t i16 = 16;
        s.setAttribute("int16", i16);
        int32_t i32 = 32;
        s.setAttribute("int32", i32);
        int64_t i64 = 64;
        s.setAttribute("int64", i64);
        uint16_t u16 = 16u;
        s.setAttribute("uint16", u16);
        uint32_t u32 = 32u;
        s.setAttribute("uint32", u32);
        uint64_t u64 = 64u;
        s.setAttribute("uint64", u64);
        float f = 16.e10f;
        s.setAttribute("float", f);
        double d = 1.e64;
        s.setAttribute("double", d);
        long double l = 1.e80L;
        s.setAttribute("longdouble", l);
        std::string str = "string";
        s.setAttribute("string", str);
        s.setAttribute("vecChar", std::vector< char >({'c', 'h', 'a', 'r'}));
        s.setAttribute("vecInt16", std::vector< int16_t >({32766, 32767}));
        s.setAttribute("vecInt32", std::vector< int32_t >({2147483646, 2147483647}));
        s.setAttribute("vecInt64", std::vector< int64_t >({9223372036854775806, 9223372036854775807}));
        s.setAttribute("vecUchar", std::vector< char >({'u', 'c', 'h', 'a', 'r'}));
        s.setAttribute("vecUint16", std::vector< uint16_t >({65534u, 65535u}));
        s.setAttribute("vecUint32", std::vector< uint32_t >({4294967294u, 4294967295u}));
        s.setAttribute("vecUint64", std::vector< uint64_t >({18446744073709551614u, 18446744073709551615u}));
        s.setAttribute("vecFloat", std::vector< float >({0.f, 3.40282e+38f}));
        s.setAttribute("vecDouble", std::vector< double >({0., 1.79769e+308}));
        s.setAttribute("vecLongdouble", std::vector< long double >({0.L, std::numeric_limits<long double>::max()}));
        s.setAttribute("vecString", std::vector< std::string >({"vector", "of", "strings"}));
        s.setAttribute("bool", true);
    }

    Series s = Series("../samples/dtype_test.bp", AccessType::READ_ONLY);

    REQUIRE(s.getAttribute("char").get< char >() == 'c');
    REQUIRE(s.getAttribute("uchar").get< unsigned char >() == 'u');
    REQUIRE(s.getAttribute("int16").get< int16_t >() == 16);
    REQUIRE(s.getAttribute("int32").get< int32_t >() == 32);
    REQUIRE(s.getAttribute("int64").get< int64_t >() == 64);
    REQUIRE(s.getAttribute("uint16").get< uint16_t >() == 16u);
    REQUIRE(s.getAttribute("uint32").get< uint32_t >() == 32u);
    REQUIRE(s.getAttribute("uint64").get< uint64_t >() == 64u);
    REQUIRE(s.getAttribute("float").get< float >() == 16.e10f);
    REQUIRE(s.getAttribute("double").get< double >() == 1.e64);
#if !defined(_MSC_VER)
    REQUIRE(s.getAttribute("longdouble").get< long double >() == 1.e80L);
#endif
    REQUIRE(s.getAttribute("string").get< std::string >() == "string");
    REQUIRE(s.getAttribute("vecChar").get< std::vector< char > >() == std::vector< char >({'c', 'h', 'a', 'r'}));
    REQUIRE(s.getAttribute("vecInt16").get< std::vector< int16_t > >() == std::vector< int16_t >({32766, 32767}));
    REQUIRE(s.getAttribute("vecInt32").get< std::vector< int32_t > >() == std::vector< int32_t >({2147483646, 2147483647}));
    REQUIRE(s.getAttribute("vecInt64").get< std::vector< int64_t > >() == std::vector< int64_t >({9223372036854775806, 9223372036854775807}));
    REQUIRE(s.getAttribute("vecUchar").get< std::vector< char > >() == std::vector< char >({'u', 'c', 'h', 'a', 'r'}));
    REQUIRE(s.getAttribute("vecUint16").get< std::vector< uint16_t > >() == std::vector< uint16_t >({65534u, 65535u}));
    REQUIRE(s.getAttribute("vecUint32").get< std::vector< uint32_t > >() == std::vector< uint32_t >({4294967294u, 4294967295u}));
    REQUIRE(s.getAttribute("vecUint64").get< std::vector< uint64_t > >() == std::vector< uint64_t >({18446744073709551614u, 18446744073709551615u}));
    REQUIRE(s.getAttribute("vecFloat").get< std::vector< float > >() == std::vector< float >({0.f, 3.40282e+38f}));
    REQUIRE(s.getAttribute("vecDouble").get< std::vector< double > >() == std::vector< double >({0., 1.79769e+308}));
#if !defined(_MSC_VER)
    REQUIRE(s.getAttribute("vecLongdouble").get< std::vector< long double > >() == std::vector< long double >({0.L, std::numeric_limits<long double>::max()}));
#endif
    REQUIRE(s.getAttribute("vecString").get< std::vector< std::string > >() == std::vector< std::string >({"vector", "of", "strings"}));
    REQUIRE(s.getAttribute("bool").get< uint8_t >() == static_cast< uint8_t >(true));
}

TEST_CASE( "adios1_write_test", "[serial][adios1]")
{
    Series o = Series("../samples/serial_write.bp", AccessType::CREATE);

    ParticleSpecies& e_1 = o.iterations[1].particles["e"];

    std::vector< double > position_global(4);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local_1(new double);
    e_1["position"]["x"].resetDataset(Dataset(determineDatatype(position_local_1), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local_1 = position_global[i];
        e_1["position"]["x"].storeChunk({i}, {1}, position_local_1);
        o.flush();
    }

    std::vector< uint64_t > positionOffset_global(4);
    uint64_t posOff{0};
    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_1(new uint64_t);
    e_1["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_1), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_1 = positionOffset_global[i];
        e_1["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_1);
        o.flush();
    }

    ParticleSpecies& e_2 = o.iterations[2].particles["e"];

    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local_2(new double);
    e_2["position"]["x"].resetDataset(Dataset(determineDatatype(position_local_2), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local_2 = position_global[i];
        e_2["position"]["x"].storeChunk({i}, {1}, position_local_2);
        o.flush();
    }

    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_2(new uint64_t);
    e_2["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_2), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_2 = positionOffset_global[i];
        e_2["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_2);
        o.flush();
    }

    o.flush();

    ParticleSpecies& e_3 = o.iterations[3].particles["e"];

    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local_3(new double);
    e_3["position"]["x"].resetDataset(Dataset(determineDatatype(position_local_3), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local_3 = position_global[i];
        e_3["position"]["x"].storeChunk({i}, {1}, position_local_3);
        o.flush();
    }

    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_3(new uint64_t);
    e_3["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_3), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_3 = positionOffset_global[i];
        e_3["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_3);
        o.flush();
    }

    o.flush();
}

TEST_CASE( "adios1_fileBased_write_test", "[serial][adios1]" )
{
    Series o = Series("../samples/serial_fileBased_write%T.bp", AccessType::CREATE);

    ParticleSpecies& e_1 = o.iterations[1].particles["e"];

    std::vector< double > position_global(4);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local_1(new double);
    e_1["position"]["x"].resetDataset(Dataset(determineDatatype(position_local_1), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local_1 = position_global[i];
        e_1["position"]["x"].storeChunk({i}, {1}, position_local_1);
        o.flush();
    }

    std::vector< uint64_t > positionOffset_global(4);
    uint64_t posOff{0};
    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_1(new uint64_t);
    e_1["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_1), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_1 = positionOffset_global[i];
        e_1["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_1);
        o.flush();
    }

    ParticleSpecies& e_2 = o.iterations[2].particles["e"];

    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local_2(new double);
    e_2["position"]["x"].resetDataset(Dataset(determineDatatype(position_local_2), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local_2 = position_global[i];
        e_2["position"]["x"].storeChunk({i}, {1}, position_local_2);
        o.flush();
    }

    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_2(new uint64_t);
    e_2["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_2), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_2 = positionOffset_global[i];
        e_2["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_2);
        o.flush();
    }

    o.flush();

    ParticleSpecies& e_3 = o.iterations[3].particles["e"];

    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local_3(new double);
    e_3["position"]["x"].resetDataset(Dataset(determineDatatype(position_local_3), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local_3 = position_global[i];
        e_3["position"]["x"].storeChunk({i}, {1}, position_local_3);
        o.flush();
    }

    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local_3(new uint64_t);
    e_3["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local_3), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local_3 = positionOffset_global[i];
        e_3["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local_3);
        o.flush();
    }

    o.flush();
}

TEST_CASE( "hzdr_adios1_sample_content_test", "[serial][adios1]" )
{
    // since this file might not be publicly available, gracefully handle errors
    /** @todo add bp example files to https://github.com/openPMD/openPMD-example-datasets */
    try
    {
        /* development/huebl/lwfa-bgfield-001 */
        Series o = Series("../samples/hzdr-sample/bp/checkpoint_%T.bp", AccessType::READ_ONLY);

        REQUIRE(o.openPMD() == "1.0.0");
        REQUIRE(o.openPMDextension() == 1);
        REQUIRE(o.basePath() == "/data/%T/");
        REQUIRE(o.meshesPath() == "fields/");
        REQUIRE(o.particlesPath() == "particles/");
        REQUIRE(o.author() == "Axel Huebl <a.huebl@hzdr.de>");
        REQUIRE(o.software() == "PIConGPU");
        REQUIRE(o.softwareVersion() == "0.4.0-dev");
        REQUIRE(o.date() == "2017-07-14 19:29:13 +0200");
        REQUIRE(o.iterationEncoding() == IterationEncoding::fileBased);
        REQUIRE(o.iterationFormat() == "checkpoint_%T.bp");

        REQUIRE(o.iterations.size() >= 1);
        REQUIRE(o.iterations.count(0) == 1);

        Iteration& i = o.iterations[0];
        REQUIRE(i.time< float >() == static_cast< float >(0.0f));
        REQUIRE(i.dt< float >() == static_cast< float >(1.0f));
        REQUIRE(i.timeUnitSI() == 1.3899999999999999e-16);

        REQUIRE(i.meshes.count("B") == 1);
        REQUIRE(i.meshes.count("E") == 1);
        REQUIRE(i.meshes.size() == 2);

        std::vector< std::string > al{"z", "y", "x"};
        std::vector< float > gs{static_cast< float >(4.252342224121094f),
                                static_cast< float >(1.0630855560302734f),
                                static_cast< float >(4.252342224121094f)};
        std::vector< double > ggo{0., 0., 0.};
        std::array< double, 7 > ud{{0.,  1., -2., -1.,  0.,  0.,  0.}};
        Mesh& B = i.meshes["B"];
        REQUIRE(B.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(B.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(B.axisLabels() == al);
        REQUIRE(B.gridSpacing< float >() == gs);
        REQUIRE(B.gridGlobalOffset() == ggo);
        REQUIRE(B.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(B.unitDimension() == ud);
        REQUIRE(B.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(B.size() == 3);
        REQUIRE(B.count("x") == 1);
        REQUIRE(B.count("y") == 1);
        REQUIRE(B.count("z") == 1);

        std::vector< float > p{static_cast< float >(0.0f),
                               static_cast< float >(0.5f),
                               static_cast< float >(0.5f)};
        Extent e{192, 512, 192};
        MeshRecordComponent& B_x = B["x"];
        REQUIRE(B_x.unitSI() == 40903.82224060171);
        REQUIRE(B_x.position< float >() == p);
        REQUIRE(B_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_x.getExtent() == e);
        REQUIRE(B_x.getDimensionality() == 3);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.0f),
             static_cast< float >(0.5f)};
        MeshRecordComponent& B_y = B["y"];
        REQUIRE(B_y.unitSI() == 40903.82224060171);
        REQUIRE(B_y.position< float >() == p);
        REQUIRE(B_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_y.getExtent() == e);
        REQUIRE(B_y.getDimensionality() == 3);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.5f),
             static_cast< float >(0.0f)};
        MeshRecordComponent& B_z = B["z"];
        REQUIRE(B_z.unitSI() == 40903.82224060171);
        REQUIRE(B_z.position< float >() == p);
        REQUIRE(B_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(B_z.getExtent() == e);
        REQUIRE(B_z.getDimensionality() == 3);

        ud = {{1.,  1., -3., -1.,  0.,  0.,  0.}};
        Mesh& E = i.meshes["E"];
        REQUIRE(E.geometry() == Mesh::Geometry::cartesian);
        REQUIRE(E.dataOrder() == Mesh::DataOrder::C);
        REQUIRE(E.axisLabels() == al);
        REQUIRE(E.gridSpacing< float >() == gs);
        REQUIRE(E.gridGlobalOffset() == ggo);
        REQUIRE(E.gridUnitSI() == 4.1671151661999998e-08);
        REQUIRE(E.unitDimension() == ud);
        REQUIRE(E.timeOffset< float >() == static_cast< float >(0.0f));

        REQUIRE(E.size() == 3);
        REQUIRE(E.count("x") == 1);
        REQUIRE(E.count("y") == 1);
        REQUIRE(E.count("z") == 1);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.0f),
             static_cast< float >(0.0f)};
        e = {192, 512, 192};
        MeshRecordComponent& E_x = E["x"];
        REQUIRE(E_x.unitSI() == 12262657411105.05);
        REQUIRE(E_x.position< float >() == p);
        REQUIRE(E_x.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_x.getExtent() == e);
        REQUIRE(E_x.getDimensionality() == 3);

        p = {static_cast< float >(0.0f),
             static_cast< float >(0.5f),
             static_cast< float >(0.0f)};
        MeshRecordComponent& E_y = E["y"];
        REQUIRE(E_y.unitSI() == 12262657411105.05);
        REQUIRE(E_y.position< float >() == p);
        REQUIRE(E_y.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_y.getExtent() == e);
        REQUIRE(E_y.getDimensionality() == 3);

        p = {static_cast< float >(0.0f),
             static_cast< float >(0.0f),
             static_cast< float >(0.5f)};
        MeshRecordComponent& E_z = E["z"];
        REQUIRE(E_z.unitSI() == 12262657411105.05);
        REQUIRE(E_z.position< float >() == p);
        REQUIRE(E_z.getDatatype() == Datatype::FLOAT);
        REQUIRE(E_z.getExtent() == e);
        REQUIRE(E_z.getDimensionality() == 3);

        REQUIRE(i.particles.empty());

        float actual[3][3][3] = {{{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                                     {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                                     {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}},
                                 {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                                     {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                                     {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}},
                                 {{6.7173387e-06f, 6.7173387e-06f, 6.7173387e-06f},
                                     {7.0438218e-06f, 7.0438218e-06f, 7.0438218e-06f},
                                     {7.3689453e-06f, 7.3689453e-06f, 7.3689453e-06f}}};
        Offset offset{20, 20, 150};
        Extent extent{3, 3, 3};
        auto data = B_z.loadChunk<float>(offset, extent);
        o.flush();
        float* raw_ptr = data.get();

        for( int i = 0; i < 3; ++i )
            for( int j = 0; j < 3; ++j )
                for( int k = 0; k < 3; ++k )
                    REQUIRE(raw_ptr[((i*3) + j)*3 + k] == actual[i][j][k]);
    } catch (no_such_file_error& e)
    {
        std::cerr << "HZDR sample not accessible. (" << e.what() << ")\n";
        return;
    }
}
#else
TEST_CASE( "no_serial_adios1", "[serial][adios]")
{
    REQUIRE(true);
}
#endif
