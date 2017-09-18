#define BOOST_TEST_MODULE libopenpmd_core_test


/* make Writable::parent, Writable::IOHandler visible for structure_test */
#define protected public
#include <boost/test/included/unit_test.hpp>

#include "../include/Output.hpp"


BOOST_AUTO_TEST_CASE(output_default_test)
{
    using IE = IterationEncoding;
    Output o = Output("./",
                      "new_openpmd_output_%T",
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
    BOOST_TEST(o.name() == "new_openpmd_output_%T");

    o.iterations[0];
}

BOOST_AUTO_TEST_CASE(output_constructor_test)
{
    using IE = IterationEncoding;
    Output o1 = Output("./",
                      "MyOutput_%T",
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
    BOOST_TEST(o1.name() == "MyOutput_%T");

    o1.iterations[0];

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
    using IE = IterationEncoding;
    Output o = Output("./",
                      "MyOutput_%T",
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

    o.iterations[0];
}

BOOST_AUTO_TEST_CASE(iteration_default_test)
{
    using IE = IterationEncoding;
    Output o = Output("./",
                      "MyOutput_%T",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Iteration& i = o.iterations[42];

    BOOST_TEST(i.time< double >() == static_cast<double>(0));
    BOOST_TEST(i.dt< double >() == static_cast<double>(1));
    BOOST_TEST(i.timeUnitSI() == static_cast<double>(1));
    BOOST_TEST(i.numAttributes() == 3);
    BOOST_TEST(i.meshes.size() == 0);
    BOOST_TEST(i.particles.size() == 0);
}

BOOST_AUTO_TEST_CASE(iteration_modification_test)
{
    using IE = IterationEncoding;
    Output o = Output("./",
                      "MyOutput_%T",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Iteration& i = o.iterations[42];

    float time = 0.314;
    i.setTime(time);
    BOOST_TEST(i.time< float >() == time);

    double dt = 0.42;
    i.setDt(dt);
    BOOST_TEST(i.dt< double >() == dt);

    i.setTimeUnitSI(0.000000000001);
    BOOST_TEST(i.timeUnitSI() == static_cast< double >(0.000000000001));
}


BOOST_AUTO_TEST_CASE(record_constructor_test)
{
    using IE = IterationEncoding;
    Output o = Output("./",
                      "MyOutput_%T",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Record& r = o.iterations[42].particles["species"]["record"];

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
    BOOST_TEST(r.timeOffset< float >() == static_cast<float>(0));
    BOOST_TEST(r.numAttributes() == 2); /* timeOffset, unitDimension */
}

BOOST_AUTO_TEST_CASE(record_modification_test)
{
    using IE = IterationEncoding;
    Output o = Output("./",
                      "MyOutput_%T",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Record& r = o.iterations[42].particles["species"]["record"];

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

    float timeOffset = 0.314;
    r.setTimeOffset(timeOffset);
    BOOST_TEST(r.timeOffset< float >() == timeOffset);
}

BOOST_AUTO_TEST_CASE(recordComponent_modification_test)
{
    using IE = IterationEncoding;
    Output o = Output("./",
                      "MyOutput_%T",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Record& r = o.iterations[42].particles["species"]["record"];

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

BOOST_AUTO_TEST_CASE(mesh_constructor_test)
{
    using IE = IterationEncoding;
    Output o = Output("./",
                      "MyOutput_%T",
                      IE::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    Mesh &m = o.iterations[42].meshes["E"];

    std::vector< double > pos{0};
    BOOST_TEST(m["x"].unitSI() == 1);
    BOOST_TEST(m["x"].numAttributes() == 2); /* unitSI, position */
    BOOST_TEST(m["x"].position< double >() == pos);
    BOOST_TEST(m["y"].unitSI() == 1);
    BOOST_TEST(m["y"].numAttributes() == 2); /* unitSI, position */
    BOOST_TEST(m["y"].position< double >() == pos);
    BOOST_TEST(m["z"].unitSI() == 1);
    BOOST_TEST(m["z"].numAttributes() == 2); /* unitSI, position */
    BOOST_TEST(m["z"].position< double >() == pos);
    BOOST_TEST(m.geometry() == Mesh::Geometry::cartesian);
    BOOST_TEST(m.dataOrder() == Mesh::DataOrder::C);
    std::vector< std::string > al{""};
    BOOST_TEST(m.axisLabels() == al);
    std::vector< double > gs{1};
    BOOST_TEST(m.gridSpacing< double >() == gs);
    std::vector< double > ggo{0};
    BOOST_TEST(m.gridGlobalOffset() == ggo);
    BOOST_TEST(m.gridUnitSI() == static_cast<double>(1));
    BOOST_TEST(m.numAttributes() == 8); /* axisLabels, dataOrder, geometry, gridGlobalOffset, gridSpacing, gridUnitSI, timeOffset, unitDimension */
}

BOOST_AUTO_TEST_CASE(mesh_modification_test)
{
    using IE = IterationEncoding;
    Output o = Output("./",
                      "MyOutput_%T",
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
    std::vector< double > gs{1e-5, 2e-5, 3e-5};
    m.setGridSpacing(gs);
    BOOST_TEST(m.gridSpacing< double >() == gs);
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

    m["x"].setPosition(std::vector< float >{0, 0, 0});
    BOOST_TEST(m.numAttributes() == 9);
}

BOOST_AUTO_TEST_CASE(structure_test)
{
    Output o = Output("./",
                      "new_openpmd_output_%T",
                      IterationEncoding::fileBased,
                      Format::NONE,
                      AccessType::CREAT);

    BOOST_TEST(o.IOHandler);
    BOOST_TEST(o.iterations.IOHandler);
    BOOST_TEST(o.parent == nullptr);
    BOOST_TEST(o.iterations.parent == static_cast< Writable* >(&o));

    Iteration i = o.iterations[1];
    BOOST_TEST(i.IOHandler);
    BOOST_TEST(o.iterations[1].IOHandler);
    BOOST_TEST(i.parent == static_cast< Writable* >(&o.iterations));
    BOOST_TEST(o.iterations[1].parent == static_cast< Writable* >(&o.iterations));

    Mesh m = o.iterations[1].meshes["M"];
    BOOST_TEST(m.IOHandler);
    BOOST_TEST(o.iterations[1].meshes["M"].IOHandler);
    BOOST_TEST(m.parent == static_cast< Writable* >(&o.iterations[1].meshes));
    BOOST_TEST(o.iterations[1].meshes["M"].parent == static_cast< Writable* >(&o.iterations[1].meshes));

    MeshRecordComponent mrc = o.iterations[1].meshes["M"]["MRC"];
    BOOST_TEST(mrc.IOHandler);
    BOOST_TEST(o.iterations[1].meshes["M"]["MRC"].IOHandler);
    BOOST_TEST(mrc.parent == static_cast< Writable* >(&o.iterations[1].meshes["M"]));
    BOOST_TEST(o.iterations[1].meshes["M"]["MRC"].parent == static_cast< Writable* >(&o.iterations[1].meshes["M"]));
    mrc = o.iterations[1].meshes["M"]["MRC"].makeConstant(1.0);
    BOOST_TEST(mrc.IOHandler);
    BOOST_TEST(o.iterations[1].meshes["M"]["MRC"].IOHandler);
    BOOST_TEST(mrc.parent == static_cast< Writable* >(&o.iterations[1].meshes["M"]));
    BOOST_TEST(o.iterations[1].meshes["M"]["MRC"].parent == static_cast< Writable* >(&o.iterations[1].meshes["M"]));

    MeshRecordComponent scalar_mrc = o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR];
    BOOST_TEST(scalar_mrc.IOHandler);
    BOOST_TEST(o.iterations[1].meshes["M2"].IOHandler);
    BOOST_TEST(o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].IOHandler);
    BOOST_TEST(scalar_mrc.parent == static_cast< Writable* >(&o.iterations[1].meshes));
    BOOST_TEST(o.iterations[1].meshes["M2"].parent == static_cast< Writable* >(&o.iterations[1].meshes));
    BOOST_TEST(o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].parent == static_cast< Writable* >(&o.iterations[1].meshes));
    scalar_mrc = o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].makeConstant(1.0);
    BOOST_TEST(scalar_mrc.IOHandler);
    BOOST_TEST(o.iterations[1].meshes["M2"].IOHandler);
    BOOST_TEST(o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].IOHandler);
    BOOST_TEST(scalar_mrc.parent == static_cast< Writable* >(&o.iterations[1].meshes));
    BOOST_TEST(o.iterations[1].meshes["M2"].parent == static_cast< Writable* >(&o.iterations[1].meshes));
    BOOST_TEST(o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].parent == static_cast< Writable* >(&o.iterations[1].meshes));

    ParticleSpecies ps = o.iterations[1].particles["P"];
    BOOST_TEST(ps.IOHandler);
    BOOST_TEST(o.iterations[1].particles["P"].IOHandler);
    BOOST_TEST(ps.parent == static_cast< Writable* >(&o.iterations[1].particles));
    BOOST_TEST(o.iterations[1].particles["P"].parent == static_cast< Writable* >(&o.iterations[1].particles));

    Record r = o.iterations[1].particles["P"]["PR"];
    BOOST_TEST(r.IOHandler);
    BOOST_TEST(o.iterations[1].particles["P"]["PR"].IOHandler);
    BOOST_TEST(r.parent == static_cast< Writable* >(&o.iterations[1].particles["P"]));
    BOOST_TEST(o.iterations[1].particles["P"]["PR"].parent == static_cast< Writable* >(&o.iterations[1].particles["P"]));

    RecordComponent rc = o.iterations[1].particles["P"]["PR"]["PRC"];
    BOOST_TEST(rc.IOHandler);
    BOOST_TEST(o.iterations[1].particles["P"]["PR"]["PRC"].IOHandler);
    BOOST_TEST(rc.parent == static_cast< Writable* >(&o.iterations[1].particles["P"]["PR"]));
    BOOST_TEST(o.iterations[1].particles["P"]["PR"]["PRC"].parent == static_cast< Writable* >(&o.iterations[1].particles["P"]["PR"]));
    rc = o.iterations[1].particles["P"]["PR"]["PRC"].makeConstant(1.0);
    BOOST_TEST(rc.IOHandler);
    BOOST_TEST(o.iterations[1].particles["P"]["PR"]["PRC"].IOHandler);
    BOOST_TEST(rc.parent == static_cast< Writable* >(&o.iterations[1].particles["P"]["PR"]));
    BOOST_TEST(o.iterations[1].particles["P"]["PR"]["PRC"].parent == static_cast< Writable* >(&o.iterations[1].particles["P"]["PR"]));

    RecordComponent scalar_rc = o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR];
    BOOST_TEST(scalar_rc.IOHandler);
    BOOST_TEST(o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].IOHandler);
    BOOST_TEST(scalar_rc.parent == static_cast< Writable* >(&o.iterations[1].particles["P"]));
    BOOST_TEST(o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].parent == static_cast< Writable* >(&o.iterations[1].particles["P"]));
    scalar_rc = o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].makeConstant(1.0);
    BOOST_TEST(scalar_rc.IOHandler);
    BOOST_TEST(o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].IOHandler);
    BOOST_TEST(scalar_rc.parent == static_cast< Writable* >(&o.iterations[1].particles["P"]));
    BOOST_TEST(o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].parent == static_cast< Writable* >(&o.iterations[1].particles["P"]));
}
