#define BOOST_TEST_MODULE libopenpmd_core_test


#include <boost/test/included/unit_test.hpp>

#include "../include/Output.hpp"


BOOST_AUTO_TEST_CASE(output_default_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "new_openpmd_output",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    BOOST_TEST(o.openPMD() == "1.0.1");
    BOOST_TEST(o.openPMDextension() == static_cast<uint32_t>(0));
    BOOST_TEST(o.basePath() == "/data/%T/");
    BOOST_TEST(o.meshesPath() == "meshes/");
    BOOST_TEST(o.particlesPath() == "particles/");
    BOOST_TEST(o.iterationEncoding() == IE::fileBased);
    BOOST_TEST(o.iterationFormat() == "new_openpmd_output_%T");
    BOOST_TEST(o.iterations.size() == 0);
    BOOST_TEST(o.numAttributes() == 7); /* openPMD, openPMDextension, basePath, meshesPath, particlesPath, iterationEncoding, iterationFormat */
    BOOST_TEST(o.name() == "new_openpmd_output");
}

BOOST_AUTO_TEST_CASE(output_constructor_test)
{
    using IE = Output::IterationEncoding;
    Output o1 = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    BOOST_TEST(o1.openPMD() == "1.0.1");
    BOOST_TEST(o1.openPMDextension() == static_cast<uint32_t>(0));
    BOOST_TEST(o1.basePath() == "/data/%T/");
    BOOST_TEST(o1.meshesPath() == "meshes/");
    BOOST_TEST(o1.particlesPath() == "particles/");
    BOOST_TEST(o1.iterationEncoding() == IE::fileBased);
    BOOST_TEST(o1.iterationFormat() == "MyOutput_%T");
    BOOST_TEST(o1.iterations.size() == 0);
    BOOST_TEST(o1.numAttributes() == 7); /* openPMD, openPMDextension, basePath, meshesPath, particlesPath, iterationEncoding, iterationFormat */
    BOOST_TEST(o1.name() == "MyOutput");

    Output o2 = Output("./",
                       "MyCustomOutput",
                       IE::groupBased,
                       Format::NONE,
                       AccessType::CREAT);

    o2.setMeshesPath("customMeshesPath").setParticlesPath("customParticlesPath");

    BOOST_TEST(o2.openPMD() == "1.0.1");
    BOOST_TEST(o2.openPMDextension() == static_cast<uint32_t>(0));
    BOOST_TEST(o2.basePath() == "/data/%T/");
    BOOST_TEST(o2.meshesPath() == "customMeshesPath/");
    BOOST_TEST(o2.particlesPath() == "customParticlesPath/");
    BOOST_TEST(o2.iterationEncoding() == IE::groupBased);
    BOOST_TEST(o2.iterationFormat() == "/data/%T/");
    BOOST_TEST(o2.iterations.size() == 0);
    BOOST_TEST(o2.numAttributes() == 7); /* openPMD, openPMDextension, basePath, meshesPath, particlesPath, iterationEncoding, iterationFormat */
    BOOST_TEST(o2.name() == "MyCustomOutput");
}

BOOST_AUTO_TEST_CASE(output_modification_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    o.setOpenPMD("1.0.0");
    BOOST_TEST(o.openPMD() == "1.0.0");

    o.setOpenPMDextension(1);
    BOOST_TEST(o.openPMDextension() == static_cast<uint32_t>(1));

    o.setMeshesPath("customMeshesPath");
    BOOST_TEST(o.meshesPath() == "customMeshesPath/");

    o.setParticlesPath("customParticlesPath");
    BOOST_TEST(o.particlesPath() == "customParticlesPath/");

    o.setIterationFormat("SomeOtherOutputScheme_%T");
    BOOST_TEST(o.iterationFormat() == "SomeOtherOutputScheme_%T");

    o.setName("MyOutput");
    BOOST_TEST(o.name() == "MyOutput");

}

BOOST_AUTO_TEST_CASE(iteration_default_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Iteration& i = o.iterations[42];

    BOOST_TEST(i.time() == static_cast<float>(0));
    BOOST_TEST(i.dt() == static_cast<float>(0));
    BOOST_TEST(i.timeUnitSI() == static_cast<double>(0));
    BOOST_TEST(i.numAttributes() == 3);
    BOOST_TEST(i.meshes.size() == 0);
    BOOST_TEST(i.particles.size() == 0);
}

BOOST_AUTO_TEST_CASE(iteration_modification_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Iteration& i = o.iterations[42];

    i.setTime(0.314);
    BOOST_TEST(i.time() == static_cast<float>(0.314));

    i.setDt(0.42);
    BOOST_TEST(i.dt() == static_cast<float>(0.42));

    i.setTimeUnitSI(0.000000000001);
    BOOST_TEST(i.timeUnitSI() == static_cast<double>(0.000000000001));
}


BOOST_AUTO_TEST_CASE(record_constructor_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Record& r = o.iterations[42].particles["e"]["position"];

    //std::vector< std::size_t > none{};
    BOOST_TEST(r["x"].unitSI() == 1);
    //BOOST_TEST(r["x"].extents == none);
    BOOST_TEST(r["x"].numAttributes() == 1); /* unitSI */
    BOOST_TEST(r["y"].unitSI() == 1);
    //BOOST_TEST(r["y"].extents == none);
    BOOST_TEST(r["y"].numAttributes() == 1); /* unitSI */
    BOOST_TEST(r["z"].unitSI() == 1);
    //BOOST_TEST(r["z"].extents == none);
    BOOST_TEST(r["z"].numAttributes() == 1); /* unitSI */
    std::array< double, 7 > zeros{{0., 0., 0., 0., 0., 0., 0.}};
    BOOST_TEST(r.unitDimension() == zeros);
    BOOST_TEST(r.timeOffset() == static_cast<float>(0));
    BOOST_TEST(r.numAttributes() == 2); /* timeOffset, unitDimension */
}

BOOST_AUTO_TEST_CASE(record_modification_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Record& r = o.iterations[42].particles["e"]["position"];

    using RUD = Record::UnitDimension;
    r.setUnitDimension({{RUD::L, 1.},
                        {RUD::M, 1.},
                        {RUD::T, -3.},
                        {RUD::I, -1.}});
    std::array< double, 7 > e_field_unitDimension{{1., 1., -3., -1., 0., 0., 0.}};
    BOOST_TEST(r.unitDimension() == e_field_unitDimension);

    r.setUnitDimension({{RUD::L, 0.},
                        {RUD::T, -2.}});
    std::array< double, 7 > b_field_unitDimension{{0., 1., -2., -1., 0., 0., 0.}};
    BOOST_TEST(r.unitDimension() == b_field_unitDimension);

    r.setTimeOffset(0.314);
    BOOST_TEST(r.timeOffset() == static_cast<float>(0.314));
}

BOOST_AUTO_TEST_CASE(recordComponent_modification_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Record& r = o.iterations[42].particles["e"]["position"];

    r["x"].setUnitSI(2.55999e-7);
    r["y"].setUnitSI(4.42999e-8);
    BOOST_TEST(r["x"].unitSI() == static_cast<double>(2.55999e-7));
    BOOST_TEST(r["x"].numAttributes() == 1); /* unitSI */
    BOOST_TEST(r["y"].unitSI() == static_cast<double>(4.42999e-8));
    BOOST_TEST(r["y"].numAttributes() == 1); /* unitSI */

    r["z"].setUnitSI(1);
    BOOST_TEST(r["z"].unitSI() == static_cast<double>(1));
    BOOST_TEST(r["z"].numAttributes() == 1); /* unitSI */
}

BOOST_AUTO_TEST_CASE(recordComponent_link_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Record& r = o.iterations[42].particles["e"]["position"];

    double arr3D[5][3][2] = {{{0,  1},  {2,  3},  {4,  5}},
                             {{6,  7},  {8,  9},  {10, 11}},
                             {{12, 13}, {14, 15}, {16, 17}},
                             {{18, 19}, {20, 21}, {22, 23}},
                             {{24, 25}, {26, 27}, {28, 29}}};
    std::vector< std::size_t > shape{{5, 3, 2}};
    //r["x"].linkDataToDisk(arr3D, {5, 3, 2});
    //BOOST_TEST(r["x"].extents == shape);
    //BOOST_TEST(*(r["x"].retrieveData<double>() + 1) == 1);
    BOOST_TEST(r["x"].unitSI() == static_cast<double>(1));
    BOOST_TEST(r["x"].numAttributes() == 1); /* unitSI */
}

BOOST_AUTO_TEST_CASE(mesh_constructor_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Mesh &m = o.iterations[42].meshes["E"];

    std::vector< double > pos = {};
    BOOST_TEST(m["x"].unitSI() == 1);
    BOOST_TEST(m["x"].numAttributes() == 2); /* unitSI, position */
    BOOST_TEST(m["x"].position() == pos);
    BOOST_TEST(m["y"].unitSI() == 1);
    BOOST_TEST(m["y"].numAttributes() == 2); /* unitSI, position */
    BOOST_TEST(m["y"].position() == pos);
    BOOST_TEST(m["z"].unitSI() == 1);
    BOOST_TEST(m["z"].numAttributes() == 2); /* unitSI, position */
    BOOST_TEST(m["z"].position() == pos);
    BOOST_TEST(m.geometry() == Mesh::Geometry::cartesian);
    BOOST_TEST(m.dataOrder() == Mesh::DataOrder::C);
    std::vector< std::string > al{};
    BOOST_TEST(m.axisLabels() == al);
    std::vector< float > gs{};
    BOOST_TEST(m.gridSpacing() == gs);
    std::vector< double > ggo{};
    BOOST_TEST(m.gridGlobalOffset() == ggo);
    BOOST_TEST(m.gridUnitSI() == static_cast<double>(1));
    BOOST_TEST(m.numAttributes() == 8); /* axisLabels, dataOrder, geometry, gridGlobalOffset, gridSpacing, gridUnitSI, timeOffset, unitDimension */
}

BOOST_AUTO_TEST_CASE(mesh_modification_test)
{
    using IE = Output::IterationEncoding;
    Output o = Output("./",
                      "MyOutput",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Mesh &m = o.iterations[42].meshes["E"];
    m["x"];
    m["y"];
    m["z"];

    m.setGeometry(Mesh::Geometry::spherical);
    BOOST_TEST(m.geometry() == Mesh::Geometry::spherical);
    BOOST_TEST(m.numAttributes() == 8);
    m.setDataOrder(Mesh::DataOrder::F);
    BOOST_TEST(m.dataOrder() == Mesh::DataOrder::F);
    BOOST_TEST(m.numAttributes() == 8);
    std::vector< std::string > al{"z_", "y_", "x_"};
    m.setAxisLabels({"z_", "y_", "x_"});
    BOOST_TEST(m.axisLabels() == al);
    BOOST_TEST(m.numAttributes() == 8);
    std::vector< float > gs{1e-5, 2e-5, 3e-5};
    m.setGridSpacing({1e-5, 2e-5, 3e-5});
    BOOST_TEST(m.gridSpacing() == gs);
    BOOST_TEST(m.numAttributes() == 8);
    std::vector< double > ggo{1e-10, 2e-10, 3e-10};
    m.setGridGlobalOffset({1e-10, 2e-10, 3e-10});
    BOOST_TEST(m.gridGlobalOffset() == ggo);
    BOOST_TEST(m.numAttributes() == 8);
    m.setGridUnitSI(42.0);
    BOOST_TEST(m.gridUnitSI() == static_cast<double>(42));
    BOOST_TEST(m.numAttributes() == 8);
    std::string gp{"FORMULA GOES HERE"};
    m.setGeometryParameters("FORMULA GOES HERE");
    BOOST_TEST(m.geometryParameters() == gp);
    BOOST_TEST(m.numAttributes() == 9);
    std::map< std::string, std::vector< double > > pos{{"x", {0, 0, 0}},
                                                       {"y", {1, 1, 1}},
                                                       {"z", {2, 2, 2}}};
    m["x"].setPosition({0, 0, 0});
    m.setPosition({{"y", {1, 1, 1}},
                   {"z", {2, 2, 2}}});
    BOOST_TEST(m.position() == pos);
    BOOST_TEST(m.numAttributes() == 9);
}
