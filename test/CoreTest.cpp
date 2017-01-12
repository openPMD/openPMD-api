#define BOOST_TEST_MODULE libopenpmd_core_test


#include <boost/test/included/unit_test.hpp>

#include "../include/Output.hpp"


BOOST_AUTO_TEST_CASE(output_default_test)
{
    Output o = Output(Output::IterationEncoding::fileBased);

    BOOST_TEST(o.name() == "new_openpmd_output");
    BOOST_TEST(o.basePath() == "/data/%T/");
    BOOST_TEST(o.meshesPath() == "meshes");
    BOOST_TEST(o.particlesPath() == "particles");
    BOOST_TEST(o.numAttributes() == 4); /* basePath, meshesPath, particlesPath, name */
    BOOST_TEST(o.iterations.size() == 0);
    BOOST_TEST(o.iterationEncoding() == Output::IterationEncoding::fileBased);
}

BOOST_AUTO_TEST_CASE(output_constructor_test)
{
    Output o1 = Output(Output::IterationEncoding::fileBased, "MyOutput");

    BOOST_TEST(o1.name() == "MyOutput");
    BOOST_TEST(o1.basePath() == "/data/%T/");
    BOOST_TEST(o1.meshesPath() == "meshes");
    BOOST_TEST(o1.particlesPath() == "particles");
    BOOST_TEST(o1.iterations.size() == 0);
    BOOST_TEST(o1.iterationEncoding() == Output::IterationEncoding::fileBased);

    Output o2 = Output(Output::IterationEncoding::fileBased,
                       "MyCustomOutput",
                       "customMeshesPath",
                       "customParticlesPath");

    BOOST_TEST(o2.name() == "MyCustomOutput");
    BOOST_TEST(o2.basePath() == "/data/%T/");
    BOOST_TEST(o2.meshesPath() == "customMeshesPath");
    BOOST_TEST(o2.particlesPath() == "customParticlesPath");
    BOOST_TEST(o2.iterations.size() == 0);
    BOOST_TEST(o2.iterationEncoding() == Output::IterationEncoding::fileBased);
}

BOOST_AUTO_TEST_CASE(output_modification_test)
{
    Output o = Output(Output::IterationEncoding::fileBased);

    o.setIterationEncoding(Output::IterationEncoding::groupBased);
    BOOST_TEST(o.iterationEncoding() == Output::IterationEncoding::groupBased);

    o.setIterationEncoding(Output::IterationEncoding::fileBased);
    BOOST_TEST(o.iterationEncoding() == Output::IterationEncoding::fileBased);

    o.setName("MyOutput");
    BOOST_TEST(o.name() == "MyOutput");

    o.setMeshesPath("customMeshesPath");
    BOOST_TEST(o.meshesPath() == "customMeshesPath");

    o.setParticlesPath("customParticlesPath");
    BOOST_TEST(o.particlesPath() == "customParticlesPath");
}

BOOST_AUTO_TEST_CASE(iteration_default_test)
{
    Iteration i = Iteration();

    BOOST_TEST(i.time() == static_cast<float>(0));
    BOOST_TEST(i.dt() == static_cast<float>(0));
    BOOST_TEST(i.timeUnitSI() == static_cast<double>(0));
    BOOST_TEST(i.numAttributes() == 3);
    BOOST_TEST(i.meshes.size() == 0);
    BOOST_TEST(i.particles.size() == 0);
}

BOOST_AUTO_TEST_CASE(iteration_constructor_test)
{
    Iteration i = Iteration(0.314, 0.42, 0.000000000001);

    BOOST_TEST(i.time() == static_cast<float>(0.314));
    BOOST_TEST(i.dt() == static_cast<float>(0.42));
    BOOST_TEST(i.timeUnitSI() == static_cast<double>(0.000000000001));
    BOOST_TEST(i.numAttributes() == 3);
}

BOOST_AUTO_TEST_CASE(iteration_modification_test)
{
    Iteration i = Iteration();

    i.setTime(0.314);
    BOOST_TEST(i.time() == static_cast<float>(0.314));

    i.setDt(0.42);
    BOOST_TEST(i.dt() == static_cast<float>(0.42));

    i.setTimeUnitSI(0.000000000001);
    BOOST_TEST(i.timeUnitSI() == static_cast<double>(0.000000000001));
}


BOOST_AUTO_TEST_CASE(record_constructor_test)
{
    using D = Record::Dimension;
    Record r = Record("1D3C", D::one, {"x", "y", "z"});

    BOOST_TEST(r.name() == "1D3C");
    BOOST_TEST(r["x"].name() == "x");
    BOOST_TEST(r["x"].unitSI() == 1);
    BOOST_TEST(r["x"].numAttributes() == 2); /* unitSI, name */
    BOOST_TEST(r["y"].name() == "y");
    BOOST_TEST(r["y"].unitSI() == 1);
    BOOST_TEST(r["y"].numAttributes() == 2); /* unitSI, name */
    BOOST_TEST(r["z"].name() == "z");
    BOOST_TEST(r["z"].unitSI() == 1);
    BOOST_TEST(r["z"].numAttributes() == 2); /* unitSI, name */
    std::array< double, 7 > empty{{0., 0., 0., 0., 0., 0., 0.}};
    BOOST_TEST(r.unitDimension() == empty);
    BOOST_TEST(r.timeOffset() == static_cast<float>(0));
    BOOST_TEST(r.numAttributes() == 3); /* timeOffset, unitDimension, name */
}

BOOST_AUTO_TEST_CASE(record_modification_test)
{
    using D = Record::Dimension;
    Record r = Record("3D3C", D::three, {"x", "y", "z"});

    r.setName("E_to_B");
    BOOST_TEST(r.name() == "E_to_B");

    using RD = Record::UnitDimension;
    r.setUnitDimension({{RD::L, 1.},
                        {RD::M, 1.},
                        {RD::T, -3.},
                        {RD::I, -1.}});
    std::array< double, 7 > e_field_unitDimension{{1., 1., -3., -1., 0., 0., 0.}};
    BOOST_TEST(r.unitDimension() == e_field_unitDimension);

    r.setUnitDimension({{RD::L, 0.},
                        {RD::T, -2.}});
    std::array< double, 7 > b_field_unitDimension{{0., 1., -2., -1., 0., 0., 0.}};
    BOOST_TEST(r.unitDimension() == b_field_unitDimension);

    r.setTimeOffset(0.314);
    BOOST_TEST(r.timeOffset() == static_cast<float>(0.314));

    r.setUnitSI({{"x", 2.55999e-7},
                 {"y", 4.42999e-8}});
    BOOST_TEST(r["x"].name() == "x");
    BOOST_TEST(r["x"].unitSI() == static_cast<double>(2.55999e-7));
    BOOST_TEST(r["x"].numAttributes() == 2); /* unitSI, name */
    BOOST_TEST(r["y"].name() == "y");
    BOOST_TEST(r["y"].unitSI() == static_cast<double>(4.42999e-8));
    BOOST_TEST(r["y"].numAttributes() == 2); /* unitSI, name */

    r["z"].setUnitSI(1);
    BOOST_TEST(r["z"].name() == "z");
    BOOST_TEST(r["z"].unitSI() == static_cast<double>(1));
    BOOST_TEST(r["z"].numAttributes() == 2); /* unitSI, name */


}

BOOST_AUTO_TEST_CASE(mesh_constructor_test)
{
    Mesh m = Record("B", Record::Dimension::three, {"x", "y", "z"});

    BOOST_TEST(m.name() == "B");
    BOOST_TEST(m["x"].name() == "x");
    BOOST_TEST(m["x"].unitSI() == 1);
    BOOST_TEST(m["x"].numAttributes() == 3); /* unitSI, position, name */
    BOOST_TEST(m["y"].name() == "y");
    BOOST_TEST(m["y"].unitSI() == 1);
    BOOST_TEST(m["y"].numAttributes() == 3); /* unitSI, position, name */
    BOOST_TEST(m["z"].name() == "z");
    BOOST_TEST(m["z"].unitSI() == 1);
    BOOST_TEST(m["z"].numAttributes() == 3); /* unitSI, position, name */
    BOOST_TEST(m.geometry() == Mesh::Geometry::cartesian);
    BOOST_TEST(m.dataOrder() == Mesh::DataOrder::C);
    std::vector< std::string > al{"z", "y", "x"};
    BOOST_TEST(m.axisLabels() == al);
    std::vector< float > gs{1, 1, 1};
    BOOST_TEST(m.gridSpacing() == gs);
    std::vector< double > ggo{0, 0, 0};
    BOOST_TEST(m.gridGlobalOffset() == ggo);
    BOOST_TEST(m.gridUnitSI() == static_cast<double>(1));
    BOOST_TEST(m.numAttributes() == 9); /* axisLabels, dataOrder, geometry, gridGlobalOffset, gridSpacing, gridUnitSI, timeOffset, unitDimension, name */
}
