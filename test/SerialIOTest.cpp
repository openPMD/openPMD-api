#define BOOST_TEST_MODULE libopenpmd_serial_io_test

/* make Writable::parent visible for hierarchy check */
#define protected public
#include "openPMD/openPMD.hpp"
#undef protected

#include <boost/test/included/unit_test.hpp>

#if defined(openPMD_HAVE_HDF5)
BOOST_AUTO_TEST_CASE(git_hdf5_sample_structure_test)
{
    try
    {
        Series o = Series::read("../samples/git-sample/data%T.h5");

        BOOST_TEST(!o.parent);
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
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

BOOST_AUTO_TEST_CASE(git_hdf5_sample_attribute_test)
{
    try
    {
        Series o = Series::read("../samples/git-sample/data%T.h5");

        BOOST_TEST(o.openPMD() == "1.1.0");
        BOOST_TEST(o.openPMDextension() == 1);
        BOOST_TEST(o.basePath() == "/data/%T/");
        BOOST_TEST(o.meshesPath() == "fields/");
        BOOST_TEST(o.particlesPath() == "particles/");
        BOOST_TEST(o.iterationEncoding() == IterationEncoding::fileBased);
        BOOST_TEST(o.iterationFormat() == "data%T.h5");
        BOOST_TEST(o.name() == "data%T");

        BOOST_TEST(o.iterations.size() == 5);
        BOOST_TEST(o.iterations.count(100) == 1);

        Iteration& iteration_100 = o.iterations[100];
        BOOST_TEST(iteration_100.time< double >() == 3.2847121452090077e-14);
        BOOST_TEST(iteration_100.dt< double >() == 3.2847121452090093e-16);
        BOOST_TEST(iteration_100.timeUnitSI() == 1.0);

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
        BOOST_TEST(E.timeOffset< double >() == 0.0);

        BOOST_TEST(E.size() == 3);
        BOOST_TEST(E.count("x") == 1);
        BOOST_TEST(E.count("y") == 1);
        BOOST_TEST(E.count("z") == 1);

        std::vector< double > p{0.5, 0., 0.};
        Extent e{26, 26, 201};
        MeshRecordComponent& E_x = E["x"];
        BOOST_TEST(E_x.unitSI() == 1.0);
        BOOST_TEST(E_x.position< double >() == p);
        BOOST_TEST(E_x.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(E_x.getExtent() == e);
        BOOST_TEST(E_x.getDimensionality() == 3);

        p = {0., 0.5, 0.};
        MeshRecordComponent& E_y = E["y"];
        BOOST_TEST(E_y.unitSI() == 1.0);
        BOOST_TEST(E_y.position< double >() == p);
        BOOST_TEST(E_y.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(E_y.getExtent() == e);
        BOOST_TEST(E_y.getDimensionality() == 3);

        p = {0., 0., 0.5};
        MeshRecordComponent& E_z = E["z"];
        BOOST_TEST(E_z.unitSI() == 1.0);
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
        BOOST_TEST(rho.timeOffset< double >() == 0.0);

        BOOST_TEST(rho.size() == 1);
        BOOST_TEST(rho.count(MeshRecordComponent::SCALAR) == 1);

        p = {0., 0., 0.};
        e = {26, 26, 201};
        MeshRecordComponent& rho_scalar = rho[MeshRecordComponent::SCALAR];
        BOOST_TEST(rho_scalar.unitSI() == 1.0);
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
        BOOST_TEST(charge.timeOffset< double >() == 0.0);

        BOOST_TEST(charge.size() == 1);
        BOOST_TEST(charge.count(RecordComponent::SCALAR) == 1);

        e = {85625};
        RecordComponent& charge_scalar = charge[RecordComponent::SCALAR];
        BOOST_TEST(charge_scalar.unitSI() == 1.0);
        BOOST_TEST(charge_scalar.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(charge_scalar.getDimensionality() == 1);
        BOOST_TEST(charge_scalar.getExtent() == e);

        ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& mass = electrons["mass"];
        BOOST_TEST(mass.unitDimension() == ud);
        BOOST_TEST(mass.timeOffset< double >() == 0.0);

        BOOST_TEST(mass.size() == 1);
        BOOST_TEST(mass.count(RecordComponent::SCALAR) == 1);

        RecordComponent& mass_scalar = mass[RecordComponent::SCALAR];
        BOOST_TEST(mass_scalar.unitSI() == 1.0);
        BOOST_TEST(mass_scalar.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(mass_scalar.getDimensionality() == 1);
        BOOST_TEST(mass_scalar.getExtent() == e);

        ud = {{1.,  1., -1.,  0.,  0.,  0.,  0.}};
        Record& momentum = electrons["momentum"];
        BOOST_TEST(momentum.unitDimension() == ud);
        BOOST_TEST(momentum.timeOffset< double >() == 0.0);

        BOOST_TEST(momentum.size() == 3);
        BOOST_TEST(momentum.count("x") == 1);
        BOOST_TEST(momentum.count("y") == 1);
        BOOST_TEST(momentum.count("z") == 1);

        RecordComponent& momentum_x = momentum["x"];
        BOOST_TEST(momentum_x.unitSI() == 1.0);
        BOOST_TEST(momentum_x.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(momentum_x.getDimensionality() == 1);
        BOOST_TEST(momentum_x.getExtent() == e);

        RecordComponent& momentum_y = momentum["y"];
        BOOST_TEST(momentum_y.unitSI() == 1.0);
        BOOST_TEST(momentum_y.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(momentum_y.getDimensionality() == 1);
        BOOST_TEST(momentum_y.getExtent() == e);

        RecordComponent& momentum_z = momentum["z"];
        BOOST_TEST(momentum_z.unitSI() == 1.0);
        BOOST_TEST(momentum_z.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(momentum_z.getDimensionality() == 1);
        BOOST_TEST(momentum_z.getExtent() == e);

        ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& position = electrons["position"];
        BOOST_TEST(position.unitDimension() == ud);
        BOOST_TEST(position.timeOffset< double >() == 0.0);

        BOOST_TEST(position.size() == 3);
        BOOST_TEST(position.count("x") == 1);
        BOOST_TEST(position.count("y") == 1);
        BOOST_TEST(position.count("z") == 1);

        RecordComponent& position_x = position["x"];
        BOOST_TEST(position_x.unitSI() == 1.0);
        BOOST_TEST(position_x.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(position_x.getDimensionality() == 1);
        BOOST_TEST(position_x.getExtent() == e);

        RecordComponent& position_y = position["y"];
        BOOST_TEST(position_y.unitSI() == 1.0);
        BOOST_TEST(position_y.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(position_y.getDimensionality() == 1);
        BOOST_TEST(position_y.getExtent() == e);

        RecordComponent& position_z = position["z"];
        BOOST_TEST(position_z.unitSI() == 1.0);
        BOOST_TEST(position_z.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(position_z.getDimensionality() == 1);
        BOOST_TEST(position_z.getExtent() == e);

        Record& positionOffset = electrons["positionOffset"];
        BOOST_TEST(positionOffset.unitDimension() == ud);
        BOOST_TEST(positionOffset.timeOffset< double >() == 0.0);

        BOOST_TEST(positionOffset.size() == 3);
        BOOST_TEST(positionOffset.count("x") == 1);
        BOOST_TEST(positionOffset.count("y") == 1);
        BOOST_TEST(positionOffset.count("z") == 1);

        RecordComponent& positionOffset_x = positionOffset["x"];
        BOOST_TEST(positionOffset_x.unitSI() == 1.0);
        BOOST_TEST(positionOffset_x.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(positionOffset_x.getDimensionality() == 1);
        BOOST_TEST(positionOffset_x.getExtent() == e);

        RecordComponent& positionOffset_y = positionOffset["y"];
        BOOST_TEST(positionOffset_y.unitSI() == 1.0);
        BOOST_TEST(positionOffset_y.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(positionOffset_y.getDimensionality() == 1);
        BOOST_TEST(positionOffset_y.getExtent() == e);

        RecordComponent& positionOffset_z = positionOffset["z"];
        BOOST_TEST(positionOffset_z.unitSI() == 1.0);
        BOOST_TEST(positionOffset_z.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(positionOffset_z.getDimensionality() == 1);
        BOOST_TEST(positionOffset_z.getExtent() == e);

        ud = {{0.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& weighting = electrons["weighting"];
        BOOST_TEST(weighting.unitDimension() == ud);
        BOOST_TEST(weighting.timeOffset< double >() == 0.0);

        BOOST_TEST(weighting.size() == 1);
        BOOST_TEST(weighting.count(RecordComponent::SCALAR) == 1);

        RecordComponent& weighting_scalar = weighting[RecordComponent::SCALAR];
        BOOST_TEST(weighting_scalar.unitSI() == 1.0);
        BOOST_TEST(weighting_scalar.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(weighting_scalar.getDimensionality() == 1);
        BOOST_TEST(weighting_scalar.getExtent() == e);
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

BOOST_AUTO_TEST_CASE(git_hdf5_sample_content_test)
{
    try
    {
        Series o = Series::read("../samples/git-sample/data%T.h5");

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
            std::unique_ptr< double[] > data;
            rho.loadChunk(offset, extent, data, RecordComponent::Allocation::API);
            double* raw_ptr = data.get();

            for( int i = 0; i < 3; ++i )
                for( int j = 0; j < 3; ++j )
                    for( int k = 0; k < 3; ++k )
                        BOOST_TEST(raw_ptr[((i*3) + j)*3 + k] == actual[i][j][k]);
        }

        {
            double constant_value = 9.1093829099999999e-31;
            RecordComponent& electrons_mass = o.iterations[100].particles["electrons"]["mass"][RecordComponent::SCALAR];
            Offset offset{15};
            Extent extent{3};
            std::unique_ptr< double[] > data;
            electrons_mass.loadChunk(offset, extent, data, RecordComponent::Allocation::API);
            double* raw_ptr = data.get();

            for( int i = 0; i < 3; ++i )
                BOOST_TEST(raw_ptr[i] == constant_value);
        }
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

BOOST_AUTO_TEST_CASE(git_hdf5_sample_fileBased_read_test)
{
    try
    {
        Series o = Series::read("../samples/git-sample/data%T.h5");

        BOOST_TEST(o.iterations.size() == 5);
        BOOST_TEST(o.iterations.count(100) == 1);
        BOOST_TEST(o.iterations.count(200) == 1);
        BOOST_TEST(o.iterations.count(300) == 1);
        BOOST_TEST(o.iterations.count(400) == 1);
        BOOST_TEST(o.iterations.count(500) == 1);
    } catch (no_such_file_error& e)
    {
        std::cerr << "git sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

BOOST_AUTO_TEST_CASE(hzdr_hdf5_sample_content_test)
{
    // since this file might not be publicly available, gracefully handle errors
    try
    {
        /* development/huebl/lwfa-openPMD-062-smallLWFA-h5 */
        Series o = Series::read("../samples/hzdr-sample/simData_%T.h5");

        BOOST_TEST(o.openPMD() == "1.0.0");
        BOOST_TEST(o.openPMDextension() == 1);
        BOOST_TEST(o.basePath() == "/data/%T/");
        BOOST_TEST(o.meshesPath() == "fields/");
        BOOST_TEST(o.particlesPath() == "particles/");
        BOOST_TEST(o.author() == "Axel Huebl <a.huebl@hzdr.de>");
        BOOST_TEST(o.software() == "PIConGPU");
        BOOST_TEST(o.softwareVersion() == "0.2.0");
        BOOST_TEST(o.date() == "2016-11-04 00:59:14 +0100");
        BOOST_TEST(o.iterationEncoding() == IterationEncoding::fileBased);
        BOOST_TEST(o.iterationFormat() == "h5/simData_%T.h5");
        BOOST_TEST(o.name() == "simData_0");

        BOOST_TEST(o.iterations.size() == 1);
        BOOST_TEST(o.iterations.count(0) == 1);

        Iteration& i = o.iterations[0];
        BOOST_TEST(i.time< float >() == static_cast< float >(0.0f));
        BOOST_TEST(i.dt< float >() == static_cast< float >(1.0f));
        BOOST_TEST(i.timeUnitSI() == 1.3899999999999999e-16);

        BOOST_TEST(i.meshes.size() == 4);
        BOOST_TEST(i.meshes.count("B") == 1);
        BOOST_TEST(i.meshes.count("E") == 1);
        BOOST_TEST(i.meshes.count("e_chargeDensity") == 1);
        BOOST_TEST(i.meshes.count("e_energyDensity") == 1);

        std::vector< std::string > al{"z", "y", "x"};
        std::vector< float > gs{static_cast< float >(6.2393283843994141f),
                                static_cast< float >(1.0630855560302734f),
                                static_cast< float >(6.2393283843994141f)};
        std::vector< double > ggo{0., 0., 0.};
        std::array< double, 7 > ud{{0.,  1., -2., -1.,  0.,  0.,  0.}};
        Mesh& B = i.meshes["B"];
        BOOST_TEST(B.geometry() == Mesh::Geometry::cartesian);
        BOOST_TEST(B.dataOrder() == Mesh::DataOrder::C);
        BOOST_TEST(B.axisLabels() == al);
        BOOST_TEST(B.gridSpacing< float >() == gs);
        BOOST_TEST(B.gridGlobalOffset() == ggo);
        BOOST_TEST(B.gridUnitSI() == 4.1671151661999998e-08);
        BOOST_TEST(B.unitDimension() == ud);
        BOOST_TEST(B.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(B.size() == 3);
        BOOST_TEST(B.count("x") == 1);
        BOOST_TEST(B.count("y") == 1);
        BOOST_TEST(B.count("z") == 1);

        std::vector< float > p{static_cast< float >(0.0f),
                               static_cast< float >(0.5f),
                               static_cast< float >(0.5f)};
        Extent e{80, 384, 80};
        MeshRecordComponent& B_x = B["x"];
        BOOST_TEST(B_x.unitSI() == 40903.822240601701);
        BOOST_TEST(B_x.position< float >() == p);
        BOOST_TEST(B_x.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(B_x.getExtent() == e);
        BOOST_TEST(B_x.getDimensionality() == 3);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.0f),
             static_cast< float >(0.5f)};
        MeshRecordComponent& B_y = B["y"];
        BOOST_TEST(B_y.unitSI() == 40903.822240601701);
        BOOST_TEST(B_y.position< float >() == p);
        BOOST_TEST(B_y.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(B_y.getExtent() == e);
        BOOST_TEST(B_y.getDimensionality() == 3);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.5f),
             static_cast< float >(0.0f)};
        MeshRecordComponent& B_z = B["z"];
        BOOST_TEST(B_z.unitSI() == 40903.822240601701);
        BOOST_TEST(B_z.position< float >() == p);
        BOOST_TEST(B_z.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(B_z.getExtent() == e);
        BOOST_TEST(B_z.getDimensionality() == 3);

        ud = {{1.,  1., -3., -1.,  0.,  0.,  0.}};
        Mesh& E = i.meshes["E"];
        BOOST_TEST(E.geometry() == Mesh::Geometry::cartesian);
        BOOST_TEST(E.dataOrder() == Mesh::DataOrder::C);
        BOOST_TEST(E.axisLabels() == al);
        BOOST_TEST(E.gridSpacing< float >() == gs);
        BOOST_TEST(E.gridGlobalOffset() == ggo);
        BOOST_TEST(E.gridUnitSI() == 4.1671151661999998e-08);
        BOOST_TEST(E.unitDimension() == ud);
        BOOST_TEST(E.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(E.size() == 3);
        BOOST_TEST(E.count("x") == 1);
        BOOST_TEST(E.count("y") == 1);
        BOOST_TEST(E.count("z") == 1);

        p = {static_cast< float >(0.5f),
             static_cast< float >(0.0f),
             static_cast< float >(0.0f)};
        e = {80, 384, 80};
        MeshRecordComponent& E_x = E["x"];
        BOOST_TEST(E_x.unitSI() == 12262657411105.049);
        BOOST_TEST(E_x.position< float >() == p);
        BOOST_TEST(E_x.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(E_x.getExtent() == e);
        BOOST_TEST(E_x.getDimensionality() == 3);

        p = {static_cast< float >(0.0f),
             static_cast< float >(0.5f),
             static_cast< float >(0.0f)};
        MeshRecordComponent& E_y = E["y"];
        BOOST_TEST(E_y.unitSI() == 12262657411105.049);
        BOOST_TEST(E_y.position< float >() == p);
        BOOST_TEST(E_y.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(E_y.getExtent() == e);
        BOOST_TEST(E_y.getDimensionality() == 3);

        p = {static_cast< float >(0.0f),
             static_cast< float >(0.0f),
             static_cast< float >(0.5f)};
        MeshRecordComponent& E_z = E["z"];
        BOOST_TEST(E_z.unitSI() == 12262657411105.049);
        BOOST_TEST(E_z.position< float >() == p);
        BOOST_TEST(E_z.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(E_z.getExtent() == e);
        BOOST_TEST(E_z.getDimensionality() == 3);

        ud = {{-3.,  0.,  1.,  1.,  0.,  0.,  0.}};
        Mesh& e_chargeDensity = i.meshes["e_chargeDensity"];
        BOOST_TEST(e_chargeDensity.geometry() == Mesh::Geometry::cartesian);
        BOOST_TEST(e_chargeDensity.dataOrder() == Mesh::DataOrder::C);
        BOOST_TEST(e_chargeDensity.axisLabels() == al);
        BOOST_TEST(e_chargeDensity.gridSpacing< float >() == gs);
        BOOST_TEST(e_chargeDensity.gridGlobalOffset() == ggo);
        BOOST_TEST(e_chargeDensity.gridUnitSI() == 4.1671151661999998e-08);
        BOOST_TEST(e_chargeDensity.unitDimension() == ud);
        BOOST_TEST(e_chargeDensity.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(e_chargeDensity.size() == 1);
        BOOST_TEST(e_chargeDensity.count(MeshRecordComponent::SCALAR) == 1);

        p = {static_cast< float >(0.f),
             static_cast< float >(0.f),
             static_cast< float >(0.f)};
        MeshRecordComponent& e_chargeDensity_scalar = e_chargeDensity[MeshRecordComponent::SCALAR];
        BOOST_TEST(e_chargeDensity_scalar.unitSI() == 66306201.002331272);
        BOOST_TEST(e_chargeDensity_scalar.position< float >() == p);
        BOOST_TEST(e_chargeDensity_scalar.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_chargeDensity_scalar.getExtent() == e);
        BOOST_TEST(e_chargeDensity_scalar.getDimensionality() == 3);

        ud = {{-1.,  1., -2.,  0.,  0.,  0.,  0.}};
        Mesh& e_energyDensity = i.meshes["e_energyDensity"];
        BOOST_TEST(e_energyDensity.geometry() == Mesh::Geometry::cartesian);
        BOOST_TEST(e_energyDensity.dataOrder() == Mesh::DataOrder::C);
        BOOST_TEST(e_energyDensity.axisLabels() == al);
        BOOST_TEST(e_energyDensity.gridSpacing< float >() == gs);
        BOOST_TEST(e_energyDensity.gridGlobalOffset() == ggo);
        BOOST_TEST(e_energyDensity.gridUnitSI() == 4.1671151661999998e-08);
        BOOST_TEST(e_energyDensity.unitDimension() == ud);
        BOOST_TEST(e_energyDensity.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(e_energyDensity.size() == 1);
        BOOST_TEST(e_energyDensity.count(MeshRecordComponent::SCALAR) == 1);

        MeshRecordComponent& e_energyDensity_scalar = e_energyDensity[MeshRecordComponent::SCALAR];
        BOOST_TEST(e_energyDensity_scalar.unitSI() == 1.0146696675429705e+18);
        BOOST_TEST(e_energyDensity_scalar.position< float >() == p);
        BOOST_TEST(e_energyDensity_scalar.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_energyDensity_scalar.getExtent() == e);
        BOOST_TEST(e_energyDensity_scalar.getDimensionality() == 3);

        BOOST_TEST(i.particles.size() == 1);
        BOOST_TEST(i.particles.count("e") == 1);

        ParticleSpecies& species_e = i.particles["e"];

        BOOST_TEST(species_e.size() == 6);
        BOOST_TEST(species_e.count("charge") == 1);
        BOOST_TEST(species_e.count("mass") == 1);
        BOOST_TEST(species_e.count("momentum") == 1);
        BOOST_TEST(species_e.count("particlePatches") == 0);
        BOOST_TEST(species_e.count("position") == 1);
        BOOST_TEST(species_e.count("positionOffset") == 1);
        BOOST_TEST(species_e.count("weighting") == 1);
        BOOST_TEST(species_e.particlePatches.size() == 4);
        BOOST_TEST(species_e.particlePatches.count("extent") == 1);
        BOOST_TEST(species_e.particlePatches.count("numParticles") == 1);
        BOOST_TEST(species_e.particlePatches.count("numParticlesOffset") == 1);
        BOOST_TEST(species_e.particlePatches.count("offset") == 1);

        ud = {{0.,  0.,  1.,  1.,  0.,  0.,  0.}};
        Record& e_charge = species_e["charge"];
        BOOST_TEST(e_charge.unitDimension() == ud);
        BOOST_TEST(e_charge.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(e_charge.size() == 1);
        BOOST_TEST(e_charge.count(RecordComponent::SCALAR) == 1);

        e = {2150400};
        RecordComponent& e_charge_scalar = e_charge[RecordComponent::SCALAR];
        BOOST_TEST(e_charge_scalar.unitSI() == 4.7980045488500004e-15);
        BOOST_TEST(e_charge_scalar.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(e_charge_scalar.getExtent() == e);
        BOOST_TEST(e_charge_scalar.getDimensionality() == 1);

        ud = {{0.,  1.,  0.,  0.,  0.,  0.,  0.}};
        Record& e_mass = species_e["mass"];
        BOOST_TEST(e_mass.unitDimension() == ud);
        BOOST_TEST(e_mass.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(e_mass.size() == 1);
        BOOST_TEST(e_mass.count(RecordComponent::SCALAR) == 1);

        RecordComponent& e_mass_scalar = e_mass[RecordComponent::SCALAR];
        BOOST_TEST(e_mass_scalar.unitSI() == 2.7279684799430467e-26);
        BOOST_TEST(e_mass_scalar.getDatatype() == Datatype::DOUBLE);
        BOOST_TEST(e_mass_scalar.getExtent() == e);
        BOOST_TEST(e_mass_scalar.getDimensionality() == 1);

        ud = {{1.,  1., -1.,  0.,  0.,  0.,  0.}};
        Record& e_momentum = species_e["momentum"];
        BOOST_TEST(e_momentum.unitDimension() == ud);
        BOOST_TEST(e_momentum.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(e_momentum.size() == 3);
        BOOST_TEST(e_momentum.count("x") == 1);
        BOOST_TEST(e_momentum.count("y") == 1);
        BOOST_TEST(e_momentum.count("z") == 1);

        RecordComponent& e_momentum_x = e_momentum["x"];
        BOOST_TEST(e_momentum_x.unitSI() == 8.1782437594864961e-18);
        BOOST_TEST(e_momentum_x.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_momentum_x.getExtent() == e);
        BOOST_TEST(e_momentum_x.getDimensionality() == 1);

        RecordComponent& e_momentum_y = e_momentum["y"];
        BOOST_TEST(e_momentum_y.unitSI() == 8.1782437594864961e-18);
        BOOST_TEST(e_momentum_y.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_momentum_y.getExtent() == e);
        BOOST_TEST(e_momentum_y.getDimensionality() == 1);

        RecordComponent& e_momentum_z = e_momentum["z"];
        BOOST_TEST(e_momentum_z.unitSI() == 8.1782437594864961e-18);
        BOOST_TEST(e_momentum_z.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_momentum_z.getExtent() == e);
        BOOST_TEST(e_momentum_z.getDimensionality() == 1);

        ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& e_position = species_e["position"];
        BOOST_TEST(e_position.unitDimension() == ud);
        BOOST_TEST(e_position.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(e_position.size() == 3);
        BOOST_TEST(e_position.count("x") == 1);
        BOOST_TEST(e_position.count("y") == 1);
        BOOST_TEST(e_position.count("z") == 1);

        RecordComponent& e_position_x = e_position["x"];
        BOOST_TEST(e_position_x.unitSI() == 2.599999993753294e-07);
        BOOST_TEST(e_position_x.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_position_x.getExtent() == e);
        BOOST_TEST(e_position_x.getDimensionality() == 1);

        RecordComponent& e_position_y = e_position["y"];
        BOOST_TEST(e_position_y.unitSI() == 4.4299999435019118e-08);
        BOOST_TEST(e_position_y.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_position_y.getExtent() == e);
        BOOST_TEST(e_position_y.getDimensionality() == 1);

        RecordComponent& e_position_z = e_position["z"];
        BOOST_TEST(e_position_z.unitSI() == 2.599999993753294e-07);
        BOOST_TEST(e_position_z.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_position_z.getExtent() == e);
        BOOST_TEST(e_position_z.getDimensionality() == 1);

        ud = {{1.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& e_positionOffset = species_e["positionOffset"];
        BOOST_TEST(e_positionOffset.unitDimension() == ud);
        BOOST_TEST(e_positionOffset.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(e_positionOffset.size() == 3);
        BOOST_TEST(e_positionOffset.count("x") == 1);
        BOOST_TEST(e_positionOffset.count("y") == 1);
        BOOST_TEST(e_positionOffset.count("z") == 1);

        RecordComponent& e_positionOffset_x = e_positionOffset["x"];
        BOOST_TEST(e_positionOffset_x.unitSI() == 2.599999993753294e-07);
        BOOST_TEST(e_positionOffset_x.getDatatype() == Datatype::INT32);
        BOOST_TEST(e_positionOffset_x.getExtent() == e);
        BOOST_TEST(e_positionOffset_x.getDimensionality() == 1);

        RecordComponent& e_positionOffset_y = e_positionOffset["y"];
        BOOST_TEST(e_positionOffset_y.unitSI() == 4.4299999435019118e-08);
        BOOST_TEST(e_positionOffset_y.getDatatype() == Datatype::INT32);
        BOOST_TEST(e_positionOffset_y.getExtent() == e);
        BOOST_TEST(e_positionOffset_y.getDimensionality() == 1);

        RecordComponent& e_positionOffset_z = e_positionOffset["z"];
        BOOST_TEST(e_positionOffset_z.unitSI() == 2.599999993753294e-07);
        BOOST_TEST(e_positionOffset_z.getDatatype() == Datatype::INT32);
        BOOST_TEST(e_positionOffset_z.getExtent() == e);
        BOOST_TEST(e_positionOffset_z.getDimensionality() == 1);

        ud = {{0.,  0.,  0.,  0.,  0.,  0.,  0.}};
        Record& e_weighting = species_e["weighting"];
        BOOST_TEST(e_weighting.unitDimension() == ud);
        BOOST_TEST(e_weighting.timeOffset< float >() == static_cast< float >(0.0f));

        BOOST_TEST(e_weighting.size() == 1);
        BOOST_TEST(e_weighting.count(RecordComponent::SCALAR) == 1);

        RecordComponent& e_weighting_scalar = e_weighting[RecordComponent::SCALAR];
        BOOST_TEST(e_weighting_scalar.unitSI() == 1.0);
        BOOST_TEST(e_weighting_scalar.getDatatype() == Datatype::FLOAT);
        BOOST_TEST(e_weighting_scalar.getExtent() == e);
        BOOST_TEST(e_weighting_scalar.getDimensionality() == 1);
    } catch (no_such_file_error& e)
    {
        std::cerr << "HZDR sample not accessible. (" << e.what() << ")\n";
        return;
    }
}

BOOST_AUTO_TEST_CASE(hdf5_dtype_test)
{
    {
        Series s = Series::create("../samples/dtype_test.h5");

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
        s.setAttribute("vecLongdouble", std::vector< long double >({0.L, 1.18973e+4932L}));
        s.setAttribute("vecString", std::vector< std::string >({"vector", "of", "strings"}));
    }
    
    Series s = Series::read("../samples/dtype_test.h5");

    BOOST_TEST(s.getAttribute("char").get< char >() == 'c');
    BOOST_TEST(s.getAttribute("uchar").get< unsigned char >() == 'u');
    BOOST_TEST(s.getAttribute("int16").get< int16_t >() == 16);
    BOOST_TEST(s.getAttribute("int32").get< int32_t >() == 32);
    BOOST_TEST(s.getAttribute("int64").get< int64_t >() == 64);
    BOOST_TEST(s.getAttribute("uint16").get< uint16_t >() == 16u);
    BOOST_TEST(s.getAttribute("uint32").get< uint32_t >() == 32u);
    BOOST_TEST(s.getAttribute("uint64").get< uint64_t >() == 64u);
    BOOST_TEST(s.getAttribute("float").get< float >() == 16.e10f);
    BOOST_TEST(s.getAttribute("double").get< double >() == 1.e64);
    BOOST_TEST(s.getAttribute("longdouble").get< long double >() == 1.e80L);
    BOOST_TEST(s.getAttribute("string").get< std::string >() == "string");
    BOOST_TEST(s.getAttribute("vecChar").get< std::vector< char > >() == std::vector< char >({'c', 'h', 'a', 'r'}));
    BOOST_TEST(s.getAttribute("vecInt16").get< std::vector< int16_t > >() == std::vector< int16_t >({32766, 32767}));
    BOOST_TEST(s.getAttribute("vecInt32").get< std::vector< int32_t > >() == std::vector< int32_t >({2147483646, 2147483647}));
    BOOST_TEST(s.getAttribute("vecInt64").get< std::vector< int64_t > >() == std::vector< int64_t >({9223372036854775806, 9223372036854775807}));
    BOOST_TEST(s.getAttribute("vecUchar").get< std::vector< char > >() == std::vector< char >({'u', 'c', 'h', 'a', 'r'}));
    BOOST_TEST(s.getAttribute("vecUint16").get< std::vector< uint16_t > >() == std::vector< uint16_t >({65534u, 65535u}));
    BOOST_TEST(s.getAttribute("vecUint32").get< std::vector< uint32_t > >() == std::vector< uint32_t >({4294967294u, 4294967295u}));
    BOOST_TEST(s.getAttribute("vecUint64").get< std::vector< uint64_t > >() == std::vector< uint64_t >({18446744073709551614u, 18446744073709551615u}));
    BOOST_TEST(s.getAttribute("vecFloat").get< std::vector< float > >() == std::vector< float >({0.f, 3.40282e+38f}));
    BOOST_TEST(s.getAttribute("vecDouble").get< std::vector< double > >() == std::vector< double >({0., 1.79769e+308}));
    BOOST_TEST(s.getAttribute("vecLongdouble").get< std::vector< long double > >() == std::vector< long double >({0.L, 1.18973e+4932L}));
    BOOST_TEST(s.getAttribute("vecString").get< std::vector< std::string > >() == std::vector< std::string >({"vector", "of", "strings"}));
}

BOOST_AUTO_TEST_CASE(hdf5_write_test)
{
    Series o = Series::create("../samples/serial_write.h5");

    o.setAuthor("Serial HDF5");
    ParticleSpecies& e = o.iterations[1].particles["e"];

    std::vector< double > position_global(4);
    double pos{0.};
    std::generate(position_global.begin(), position_global.end(), [&pos]{ return pos++; });
    std::shared_ptr< double > position_local(new double);
    e["position"]["x"].resetDataset(Dataset(determineDatatype(position_local), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *position_local = position_global[i];
        e["position"]["x"].storeChunk({i}, {1}, position_local);
        o.flush();
    }

    std::vector< uint64_t > positionOffset_global(4);
    uint64_t posOff{0};
    std::generate(positionOffset_global.begin(), positionOffset_global.end(), [&posOff]{ return posOff++; });
    std::shared_ptr< uint64_t > positionOffset_local(new uint64_t);
    e["positionOffset"]["x"].resetDataset(Dataset(determineDatatype(positionOffset_local), {4}));

    for( uint64_t i = 0; i < 4; ++i )
    {
        *positionOffset_local = positionOffset_global[i];
        e["positionOffset"]["x"].storeChunk({i}, {1}, positionOffset_local);
        o.flush();
    }

    o.flush();

    //TODO close file, read back, verify
}

BOOST_AUTO_TEST_CASE(hdf5_fileBased_write_test)
{
    Series o = Series::create("../samples/serial_fileBased_write%T.h5");

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

    //TODO close file, read back, verify
}

BOOST_AUTO_TEST_CASE(hdf5_bool_test)
{
    {
        Series o = Series::create("../samples/serial_bool.h5");

        o.setAttribute("Bool attribute (true)", true);
        o.setAttribute("Bool attribute (false)", false);
    }
    {
        Series o = Series::read("../samples/serial_bool.h5");

        auto attrs = o.attributes();
        BOOST_TEST(std::count(attrs.begin(), attrs.end(), "Bool attribute (true)") == 1);
        BOOST_TEST(std::count(attrs.begin(), attrs.end(), "Bool attribute (false)") == 1);
        BOOST_TEST(o.getAttribute("Bool attribute (true)").get< bool >() == true);
        BOOST_TEST(o.getAttribute("Bool attribute (false)").get< bool >() == false);
    }
}

BOOST_AUTO_TEST_CASE(hdf5_patch_test)
{
    Series o = Series::create("../samples/serial_patch.h5");

    o.iterations[1].particles["e"].particlePatches["offset"]["x"].setUnitSI(42);
}

BOOST_AUTO_TEST_CASE(hdf5_deletion_test)
{
    Series o = Series::create("../samples/serial_deletion.h5");


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

BOOST_AUTO_TEST_CASE(hdf5_110_optional_paths)
{
    try
    {
        {
            Series s = Series::read("../samples/issue-sample/no_fields/data%T.h5");
            auto attrs = s.attributes();
            BOOST_TEST(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
            BOOST_TEST(std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
            BOOST_TEST(s.iterations[400].meshes.size() == 0);
            BOOST_TEST(s.iterations[400].particles.size() == 1);
        }

        {
            Series s = Series::read("../samples/issue-sample/no_particles/data%T.h5");
            auto attrs = s.attributes();
            BOOST_TEST(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
            BOOST_TEST(std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
            BOOST_TEST(s.iterations[400].meshes.size() == 2);
            BOOST_TEST(s.iterations[400].particles.size() == 0);
        }
    } catch (no_such_file_error& e)
    {
        std::cerr << "issue sample not accessible. (" << e.what() << ")\n";
    }

    {
        Series s = Series::create("../samples/no_meshes_1.1.0_compliant.h5");
        s.iterations[1].particles["foo"];
    }

    {
        Series s = Series::create("../samples/no_particles_1.1.0_compliant.h5");
        s.iterations[1].meshes["foo"];
    }

    {
        Series s = Series::read("../samples/no_meshes_1.1.0_compliant.h5");
        auto attrs = s.attributes();
        BOOST_TEST(std::count(attrs.begin(), attrs.end(), "meshesPath") == 0);
        BOOST_TEST(std::count(attrs.begin(), attrs.end(), "particlesPath") == 1);
        BOOST_TEST(s.iterations[1].meshes.size() == 0);
        BOOST_TEST(s.iterations[1].particles.size() == 1);
    }

    {
        Series s = Series::read("../samples/no_particles_1.1.0_compliant.h5");
        auto attrs = s.attributes();
        BOOST_TEST(std::count(attrs.begin(), attrs.end(), "meshesPath") == 1);
        BOOST_TEST(std::count(attrs.begin(), attrs.end(), "particlesPath") == 0);
        BOOST_TEST(s.iterations[1].meshes.size() == 1);
        BOOST_TEST(s.iterations[1].particles.size() == 0);
    }
}
#else
BOOST_AUTO_TEST_CASE(no_serial_hdf5)
{
    BOOST_TEST(true);
}
#endif
#if defined(openPMD_HAVE_ADIOS1)
BOOST_AUTO_TEST_CASE(adios_write_test)
{
    Output o = Output("../samples/serial_write.bp");
}
#else
BOOST_AUTO_TEST_CASE(no_serial_adios1)
{
    BOOST_TEST(true);
}
#endif
