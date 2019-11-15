// expose private and protected members for invasive testing
#if openPMD_USE_INVASIVE_TESTS
#   define OPENPMD_private public
#   define OPENPMD_protected public
#endif
#include "openPMD/openPMD.hpp"

#include <catch2/catch.hpp>

#include <string>
#include <vector>
#include <array>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <iostream>

using namespace openPMD;


TEST_CASE( "attribute_dtype_test", "[core]" )
{
    Attribute a = Attribute(static_cast< char >(' '));
    REQUIRE(Datatype::CHAR == a.dtype);
    a = Attribute(static_cast< unsigned char >(' '));
    REQUIRE(Datatype::UCHAR == a.dtype);
    a = Attribute(static_cast< short >(0));
    REQUIRE(Datatype::SHORT == a.dtype);
    a = Attribute(static_cast< int >(0));
    REQUIRE(Datatype::INT == a.dtype);
    a = Attribute(static_cast< long >(0));
    REQUIRE(Datatype::LONG == a.dtype);
    a = Attribute(static_cast< long long >(0));
    REQUIRE(Datatype::LONGLONG == a.dtype);
    a = Attribute(static_cast< unsigned short >(0));
    REQUIRE(Datatype::USHORT == a.dtype);
    a = Attribute(static_cast< unsigned int >(0));
    REQUIRE(Datatype::UINT == a.dtype);
    a = Attribute(static_cast< unsigned long >(0));
    REQUIRE(Datatype::ULONG == a.dtype);
    a = Attribute(static_cast< unsigned long long >(0));
    REQUIRE(Datatype::ULONGLONG == a.dtype);
    a = Attribute(static_cast< float >(0.));
    REQUIRE(Datatype::FLOAT == a.dtype);
    a = Attribute(static_cast< double >(0.));
    REQUIRE(Datatype::DOUBLE == a.dtype);
    a = Attribute(static_cast< long double >(0.));
    REQUIRE(Datatype::LONG_DOUBLE == a.dtype);
    a = Attribute(std::string(""));
    REQUIRE(Datatype::STRING == a.dtype);
    a = Attribute(std::vector< char >());
    REQUIRE(Datatype::VEC_CHAR == a.dtype);
    a = Attribute(std::vector< short >());
    REQUIRE(Datatype::VEC_SHORT == a.dtype);
    a = Attribute(std::vector< int >());
    REQUIRE(Datatype::VEC_INT == a.dtype);
    a = Attribute(std::vector< long >());
    REQUIRE(Datatype::VEC_LONG == a.dtype);
    a = Attribute(std::vector< long long >());
    REQUIRE(Datatype::VEC_LONGLONG == a.dtype);
    a = Attribute(std::vector< unsigned char >());
    REQUIRE(Datatype::VEC_UCHAR == a.dtype);
    a = Attribute(std::vector< unsigned short >());
    REQUIRE(Datatype::VEC_USHORT == a.dtype);
    a = Attribute(std::vector< unsigned int >());
    REQUIRE(Datatype::VEC_UINT == a.dtype);
    a = Attribute(std::vector< unsigned long >());
    REQUIRE(Datatype::VEC_ULONG == a.dtype);
    a = Attribute(std::vector< unsigned long long >());
    REQUIRE(Datatype::VEC_ULONGLONG == a.dtype);
    a = Attribute(std::vector< float >());
    REQUIRE(Datatype::VEC_FLOAT == a.dtype);
    a = Attribute(std::vector< double >());
    REQUIRE(Datatype::VEC_DOUBLE == a.dtype);
    a = Attribute(std::vector< long double >());
    REQUIRE(Datatype::VEC_LONG_DOUBLE == a.dtype);
    a = Attribute(std::vector< std::string >());
    REQUIRE(Datatype::VEC_STRING == a.dtype);
    a = Attribute(std::array< double, 7 >());
    REQUIRE(Datatype::ARR_DBL_7 == a.dtype);
    a = Attribute(static_cast< bool >(false));
    REQUIRE(Datatype::BOOL == a.dtype);
    
    // fixed size integers
    a = Attribute(static_cast< int16_t >(0));
    REQUIRE(determineDatatype< int16_t >() == a.dtype);
    a = Attribute(static_cast< int32_t >(0));
    REQUIRE(determineDatatype< int32_t >() == a.dtype);
    a = Attribute(static_cast< int64_t >(0));
    REQUIRE(determineDatatype< int64_t >() == a.dtype);
    a = Attribute(static_cast< uint16_t >(0));
    REQUIRE(determineDatatype< uint16_t >() == a.dtype);
    a = Attribute(static_cast< uint32_t >(0));
    REQUIRE(determineDatatype< uint32_t >() == a.dtype);
    a = Attribute(static_cast< uint64_t >(0));
    REQUIRE(determineDatatype< uint64_t >() == a.dtype);
    // TODO fixed size floats

    // same implementation types (not necessary aliases) detection
    if( sizeof(long) == sizeof(long long) )
    {
        a = Attribute(static_cast< long >(0));
        REQUIRE(isSame(Datatype::LONGLONG, a.dtype));
#if !defined(_MSC_VER)
        REQUIRE(Datatype::LONGLONG == a.dtype);
#endif
        a = Attribute(static_cast< long long >(0));
        REQUIRE(isSame(Datatype::LONG, a.dtype));
#if !defined(_MSC_VER)
        REQUIRE(Datatype::LONG == a.dtype);
#endif
    }
    if( sizeof(int) == sizeof(long) )
    {
        a = Attribute(static_cast< long >(0));
        REQUIRE(isSame(Datatype::INT, a.dtype));
#if !defined(_MSC_VER)
        REQUIRE(Datatype::INT == a.dtype);
#endif
        a = Attribute(static_cast< int >(0));
        REQUIRE(isSame(Datatype::LONG, a.dtype));
#if !defined(_MSC_VER)
        REQUIRE(Datatype::LONG == a.dtype);
#endif
    }
}

TEST_CASE( "output_default_test", "[core]" )
{
    using IE = IterationEncoding;
    Series o = Series("./new_openpmd_output_%T.json", AccessType::CREATE);

    REQUIRE(o.openPMD() == "1.1.0");
    REQUIRE(o.openPMDextension() == static_cast<uint32_t>(0));
    REQUIRE(o.basePath() == "/data/%T/");
    REQUIRE(o.iterationEncoding() == IE::fileBased);
    REQUIRE(o.iterationFormat() == "new_openpmd_output_%T");
    REQUIRE(o.iterations.empty());
    REQUIRE(o.numAttributes() == 5); /* openPMD, openPMDextension, basePath, iterationEncoding, iterationFormat */
    REQUIRE(o.name() == "new_openpmd_output_%T");

    o.iterations[0];
}

TEST_CASE( "output_constructor_test", "[core]" )
{
    using IE = IterationEncoding;
    Series o = Series("./MyCustomOutput.json", AccessType::CREATE);

    o.setMeshesPath("customMeshesPath").setParticlesPath("customParticlesPath");

    o.iterations[1].meshes["foo"]["baz"].resetDataset(Dataset(Datatype::DOUBLE, {1}));
    auto species = o.iterations[1].particles["bar"];
    auto dset = Dataset(Datatype::DOUBLE, {1});
    species["position"][RecordComponent::SCALAR].resetDataset(dset);
    species["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);

    REQUIRE(o.openPMD() == "1.1.0");
    REQUIRE(o.openPMDextension() == static_cast<uint32_t>(0));
    REQUIRE(o.basePath() == "/data/%T/");
    REQUIRE(o.meshesPath() == "customMeshesPath/");
    REQUIRE(o.particlesPath() == "customParticlesPath/");
    REQUIRE(o.iterationEncoding() == IE::groupBased);
    REQUIRE(o.iterationFormat() == "/data/%T/");
    REQUIRE(o.iterations.size() == 1);
    REQUIRE(o.numAttributes() == 7); /* openPMD, openPMDextension, basePath, meshesPath, particlesPath, iterationEncoding, iterationFormat */
    REQUIRE(o.name() == "MyCustomOutput");
}

TEST_CASE( "output_modification_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    o.setOpenPMD("1.0.0");
    REQUIRE(o.openPMD() == "1.0.0");

    o.setOpenPMDextension(1);
    REQUIRE(o.openPMDextension() == static_cast<uint32_t>(1));

    o.setMeshesPath("customMeshesPath");
    REQUIRE(o.meshesPath() == "customMeshesPath/");

    o.setParticlesPath("customParticlesPath");
    REQUIRE(o.particlesPath() == "customParticlesPath/");

    o.setIterationFormat("SomeOtherOutputScheme_%T");
    REQUIRE(o.iterationFormat() == "SomeOtherOutputScheme_%T");

    o.setName("MyOutput");
    REQUIRE(o.name() == "MyOutput");

    o.iterations[0];
}

TEST_CASE( "iteration_default_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    Iteration& i = o.iterations[42];

    REQUIRE(i.time< double >() == static_cast<double>(0));
    REQUIRE(i.dt< double >() == static_cast<double>(1));
    REQUIRE(i.timeUnitSI() == static_cast<double>(1));
    REQUIRE(i.numAttributes() == 3);
    REQUIRE(i.meshes.empty());
    REQUIRE(i.particles.empty());
}

TEST_CASE( "iteration_modification_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    Iteration& i = o.iterations[42];

    float time = 0.314f;
    i.setTime(time);
    REQUIRE(i.time< float >() == time);

    double dt = 0.42;
    i.setDt(dt);
    REQUIRE(i.dt< long double >() == static_cast< long double >(dt));

    i.setTimeUnitSI(0.000000000001);
    REQUIRE(i.timeUnitSI() == static_cast< double >(0.000000000001));
}

TEST_CASE( "particleSpecies_modification_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    auto& particles = o.iterations[42].particles;
    REQUIRE(0 == particles.numAttributes());
    auto& species = particles["species"];
    REQUIRE(1 == particles.size());
    REQUIRE(1 == particles.count("species"));
    REQUIRE(0 == species.numAttributes());
    auto dset = Dataset(Datatype::DOUBLE, {1});
    species["position"][RecordComponent::SCALAR].resetDataset(dset);
    species["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);
    REQUIRE(1 == species.count("positionOffset"));
    auto& patches = species.particlePatches;
    REQUIRE(2 == patches.size());
    REQUIRE(0 == patches.numAttributes());
    auto& offset = patches["offset"];
    REQUIRE(0 == offset.size());
    REQUIRE(1 == offset.numAttributes());    //unitDimension
    std::array< double, 7 > zeros{{0., 0., 0., 0., 0., 0., 0.}};
    REQUIRE(zeros == offset.unitDimension());

    auto& off_x = offset["x"];
    off_x.resetDataset(dset);
    REQUIRE(1 == off_x.unitSI());
}


TEST_CASE( "record_constructor_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    ParticleSpecies ps = o.iterations[42].particles["species"];
    Record& r = ps["record"];
    auto dset = Dataset(Datatype::DOUBLE, {1});
    ps["position"][RecordComponent::SCALAR].resetDataset(dset);
    ps["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);

    REQUIRE(r["x"].unitSI() == 1);
    REQUIRE(r["x"].numAttributes() == 1); /* unitSI */
    REQUIRE(r["y"].unitSI() == 1);
    REQUIRE(r["y"].numAttributes() == 1); /* unitSI */
    REQUIRE(r["z"].unitSI() == 1);
    REQUIRE(r["z"].numAttributes() == 1); /* unitSI */
    std::array< double, 7 > zeros{{0., 0., 0., 0., 0., 0., 0.}};
    REQUIRE(r.unitDimension() == zeros);
    REQUIRE(r.timeOffset< float >() == static_cast<float>(0));
    REQUIRE(r.numAttributes() == 2); /* timeOffset, unitDimension */
}

TEST_CASE( "record_modification_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    auto species = o.iterations[42].particles["species"];
    Record& r = species["position"];
    auto dset = Dataset(Datatype::DOUBLE, {1});
    species["position"][RecordComponent::SCALAR].resetDataset(dset);
    species["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);

    using RUD = UnitDimension;
    r.setUnitDimension({{RUD::L, 1.},
                        {RUD::M, 1.},
                        {RUD::T, -3.},
                        {RUD::I, -1.}});
    std::array< double, 7 > e_field_unitDimension{{1., 1., -3., -1., 0., 0., 0.}};
    REQUIRE(r.unitDimension() == e_field_unitDimension);

    r.setUnitDimension({{RUD::L, 0.},
                        {RUD::T, -2.}});
    std::array< double, 7 > b_field_unitDimension{{0., 1., -2., -1., 0., 0., 0.}};
    REQUIRE(r.unitDimension() == b_field_unitDimension);

    float timeOffset = 0.314f;
    r.setTimeOffset(timeOffset);
    REQUIRE(r.timeOffset< float >() == timeOffset);
}

TEST_CASE( "recordComponent_modification_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    ParticleSpecies ps = o.iterations[42].particles["species"];
    Record& r = ps["record"];
    auto dset = Dataset(Datatype::DOUBLE, {1});
    ps["position"][RecordComponent::SCALAR].resetDataset(dset);
    ps["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);

    r["x"].setUnitSI(2.55999e-7);
    r["y"].setUnitSI(4.42999e-8);
    REQUIRE(r["x"].unitSI() == static_cast<double>(2.55999e-7));
    REQUIRE(r["x"].numAttributes() == 1); /* unitSI */
    REQUIRE(r["y"].unitSI() == static_cast<double>(4.42999e-8));
    REQUIRE(r["y"].numAttributes() == 1); /* unitSI */

    r["z"].setUnitSI(1);
    REQUIRE(r["z"].unitSI() == static_cast<double>(1));
    REQUIRE(r["z"].numAttributes() == 1); /* unitSI */
}

TEST_CASE( "mesh_constructor_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    Mesh &m = o.iterations[42].meshes["E"];

    std::vector< double > pos{0};
    REQUIRE(m["x"].unitSI() == 1);
    REQUIRE(m["x"].numAttributes() == 2); /* unitSI, position */
    REQUIRE(m["x"].position< double >() == pos);
    REQUIRE(m["y"].unitSI() == 1);
    REQUIRE(m["y"].numAttributes() == 2); /* unitSI, position */
    REQUIRE(m["y"].position< double >() == pos);
    REQUIRE(m["z"].unitSI() == 1);
    REQUIRE(m["z"].numAttributes() == 2); /* unitSI, position */
    REQUIRE(m["z"].position< double >() == pos);
    REQUIRE(m.geometry() == Mesh::Geometry::cartesian);
    REQUIRE(m.dataOrder() == Mesh::DataOrder::C);
    std::vector< std::string > al{"x"};
    REQUIRE(m.axisLabels() == al);
    std::vector< double > gs{1};
    REQUIRE(m.gridSpacing< double >() == gs);
    std::vector< double > ggo{0};
    REQUIRE(m.gridGlobalOffset() == ggo);
    REQUIRE(m.gridUnitSI() == static_cast<double>(1));
    REQUIRE(m.numAttributes() == 8); /* axisLabels, dataOrder, geometry, gridGlobalOffset, gridSpacing, gridUnitSI, timeOffset, unitDimension */
}

TEST_CASE( "mesh_modification_test", "[core]" )
{
    Series o = Series("./MyOutput_%T.json", AccessType::CREATE);

    Mesh &m = o.iterations[42].meshes["E"];
    m["x"];
    m["y"];
    m["z"];

    m.setGeometry(Mesh::Geometry::spherical);
    REQUIRE(m.geometry() == Mesh::Geometry::spherical);
    REQUIRE(m.numAttributes() == 8);
    m.setDataOrder(Mesh::DataOrder::F);
    REQUIRE(m.dataOrder() == Mesh::DataOrder::F);
    REQUIRE(m.numAttributes() == 8);
    std::vector< std::string > al{"z_", "y_", "x_"};
    m.setAxisLabels({"z_", "y_", "x_"});
    REQUIRE(m.axisLabels() == al);
    REQUIRE(m.numAttributes() == 8);
    std::vector< double > gs{1e-5, 2e-5, 3e-5};
    m.setGridSpacing(gs);
    REQUIRE(m.gridSpacing< double >() == gs);
    REQUIRE(m.numAttributes() == 8);
    std::vector< double > ggo{1e-10, 2e-10, 3e-10};
    m.setGridGlobalOffset({1e-10, 2e-10, 3e-10});
    REQUIRE(m.gridGlobalOffset() == ggo);
    REQUIRE(m.numAttributes() == 8);
    m.setGridUnitSI(42.0);
    REQUIRE(m.gridUnitSI() == static_cast<double>(42));
    REQUIRE(m.numAttributes() == 8);
    std::string gp{"FORMULA GOES HERE"};
    m.setGeometryParameters("FORMULA GOES HERE");
    REQUIRE(m.geometryParameters() == gp);
    REQUIRE(m.numAttributes() == 9);

    m["x"].setPosition(std::vector< float >{0, 0, 0});
    REQUIRE(m.numAttributes() == 9);
}

TEST_CASE( "structure_test", "[core]" )
{
#if openPMD_USE_INVASIVE_TESTS
    Series o = Series("./new_openpmd_output_%T.json", AccessType::CREATE);

    REQUIRE(o.IOHandler);
    REQUIRE(o.iterations.IOHandler);
    REQUIRE(!o.parent);
    REQUIRE(o.iterations.parent == getWritable(&o));

    Iteration i = o.iterations[1];
    REQUIRE(i.IOHandler);
    REQUIRE(o.iterations[1].IOHandler);
    REQUIRE(i.parent == getWritable(&o.iterations));
    REQUIRE(o.iterations[1].parent == getWritable(&o.iterations));

    Mesh m = o.iterations[1].meshes["M"];
    REQUIRE(m.IOHandler);
    REQUIRE(o.iterations[1].meshes["M"].IOHandler);
    REQUIRE(m.parent == getWritable(&o.iterations[1].meshes));
    REQUIRE(o.iterations[1].meshes["M"].parent == getWritable(&o.iterations[1].meshes));

    MeshRecordComponent mrc = o.iterations[1].meshes["M"]["MRC"];
    REQUIRE(mrc.IOHandler);
    REQUIRE(o.iterations[1].meshes["M"]["MRC"].IOHandler);
    REQUIRE(mrc.parent == getWritable(&o.iterations[1].meshes["M"]));
    REQUIRE(o.iterations[1].meshes["M"]["MRC"].parent == getWritable(&o.iterations[1].meshes["M"]));
    mrc = o.iterations[1].meshes["M"]["MRC"].makeConstant(1.0);
    REQUIRE(mrc.IOHandler);
    REQUIRE(o.iterations[1].meshes["M"]["MRC"].IOHandler);
    REQUIRE(mrc.parent == getWritable(&o.iterations[1].meshes["M"]));
    REQUIRE(o.iterations[1].meshes["M"]["MRC"].parent == getWritable(&o.iterations[1].meshes["M"]));

    MeshRecordComponent scalar_mrc = o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR];
    REQUIRE(scalar_mrc.IOHandler);
    REQUIRE(o.iterations[1].meshes["M2"].IOHandler);
    REQUIRE(o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].IOHandler);
    REQUIRE(scalar_mrc.parent == getWritable(&o.iterations[1].meshes));
    REQUIRE(o.iterations[1].meshes["M2"].parent == getWritable(&o.iterations[1].meshes));
    REQUIRE(o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].parent == getWritable(&o.iterations[1].meshes));
    scalar_mrc = o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].makeConstant(1.0);
    REQUIRE(scalar_mrc.IOHandler);
    REQUIRE(o.iterations[1].meshes["M2"].IOHandler);
    REQUIRE(o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].IOHandler);
    REQUIRE(scalar_mrc.parent == getWritable(&o.iterations[1].meshes));
    REQUIRE(o.iterations[1].meshes["M2"].parent == getWritable(&o.iterations[1].meshes));
    REQUIRE(o.iterations[1].meshes["M2"][MeshRecordComponent::SCALAR].parent == getWritable(&o.iterations[1].meshes));

    ParticleSpecies ps = o.iterations[1].particles["P"];
    REQUIRE(ps.IOHandler);
    REQUIRE(o.iterations[1].particles["P"].IOHandler);
    REQUIRE(ps.parent == getWritable(&o.iterations[1].particles));
    REQUIRE(o.iterations[1].particles["P"].parent == getWritable(&o.iterations[1].particles));

    REQUIRE(o.iterations[1].particles["P"].particlePatches.IOHandler);
    REQUIRE(o.iterations[1].particles["P"].particlePatches.parent == getWritable(&o.iterations[1].particles["P"]));

    auto dset = Dataset(Datatype::DOUBLE, {1});
    o.iterations[1].particles["P"]["position"][RecordComponent::SCALAR].resetDataset(dset);
    o.iterations[1].particles["P"]["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);

    Record r = o.iterations[1].particles["P"]["PR"];
    REQUIRE(r.IOHandler);
    REQUIRE(o.iterations[1].particles["P"]["PR"].IOHandler);
    REQUIRE(r.parent == getWritable(&o.iterations[1].particles["P"]));
    REQUIRE(o.iterations[1].particles["P"]["PR"].parent == getWritable(&o.iterations[1].particles["P"]));

    RecordComponent rc = o.iterations[1].particles["P"]["PR"]["PRC"];
    REQUIRE(rc.IOHandler);
    REQUIRE(o.iterations[1].particles["P"]["PR"]["PRC"].IOHandler);
    REQUIRE(rc.parent == getWritable(&o.iterations[1].particles["P"]["PR"]));
    REQUIRE(o.iterations[1].particles["P"]["PR"]["PRC"].parent == getWritable(&o.iterations[1].particles["P"]["PR"]));
    rc = o.iterations[1].particles["P"]["PR"]["PRC"].makeConstant(1.0);
    REQUIRE(rc.IOHandler);
    REQUIRE(o.iterations[1].particles["P"]["PR"]["PRC"].IOHandler);
    REQUIRE(rc.parent == getWritable(&o.iterations[1].particles["P"]["PR"]));
    REQUIRE(o.iterations[1].particles["P"]["PR"]["PRC"].parent == getWritable(&o.iterations[1].particles["P"]["PR"]));

    RecordComponent scalar_rc = o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR];
    REQUIRE(scalar_rc.IOHandler);
    REQUIRE(o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].IOHandler);
    REQUIRE(scalar_rc.parent == getWritable(&o.iterations[1].particles["P"]));
    REQUIRE(o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].parent == getWritable(&o.iterations[1].particles["P"]));
    scalar_rc = o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].makeConstant(1.0);
    REQUIRE(scalar_rc.IOHandler);
    REQUIRE(o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].IOHandler);
    REQUIRE(scalar_rc.parent == getWritable(&o.iterations[1].particles["P"]));
    REQUIRE(o.iterations[1].particles["P"]["PR2"][RecordComponent::SCALAR].parent == getWritable(&o.iterations[1].particles["P"]));

    REQUIRE(1 == o.iterations[1].particles["P"].particlePatches.count("numParticles"));
    REQUIRE(1 == o.iterations[1].particles["P"].particlePatches.count("numParticlesOffset"));

    ParticlePatches pp = o.iterations[1].particles["P"].particlePatches;
    REQUIRE(pp.IOHandler);
    REQUIRE(o.iterations[1].particles["P"].particlePatches.IOHandler);
    REQUIRE(pp.parent == getWritable(&o.iterations[1].particles["P"]));
    REQUIRE(o.iterations[1].particles["P"].particlePatches.parent == getWritable(&o.iterations[1].particles["P"]));

    PatchRecord pr = o.iterations[1].particles["P"].particlePatches["numParticles"];
    REQUIRE(pr.IOHandler);
    REQUIRE(o.iterations[1].particles["P"].particlePatches["numParticles"].IOHandler);
    REQUIRE(pr.parent == getWritable(&o.iterations[1].particles["P"].particlePatches));
    REQUIRE(o.iterations[1].particles["P"].particlePatches["numParticles"].parent == getWritable(&o.iterations[1].particles["P"].particlePatches));
    pr = o.iterations[1].particles["P"].particlePatches["extent"];
    REQUIRE(pr.IOHandler);
    REQUIRE(o.iterations[1].particles["P"].particlePatches["extent"].IOHandler);
    REQUIRE(pr.parent == getWritable(&o.iterations[1].particles["P"].particlePatches));
    REQUIRE(o.iterations[1].particles["P"].particlePatches["extent"].parent == getWritable(&o.iterations[1].particles["P"].particlePatches));

    PatchRecordComponent scalar_prc = o.iterations[1].particles["P"].particlePatches["numParticles"][RecordComponent::SCALAR];
    REQUIRE(scalar_prc.IOHandler);
    REQUIRE(o.iterations[1].particles["P"].particlePatches["numParticles"][RecordComponent::SCALAR].IOHandler);
    REQUIRE(scalar_prc.parent == getWritable(&o.iterations[1].particles["P"].particlePatches));
    REQUIRE(o.iterations[1].particles["P"].particlePatches["numParticles"][RecordComponent::SCALAR].parent == getWritable(&o.iterations[1].particles["P"].particlePatches));

    PatchRecordComponent prc = o.iterations[1].particles["P"].particlePatches["extent"]["x"];
    REQUIRE(prc.IOHandler);
    REQUIRE(o.iterations[1].particles["P"].particlePatches["extent"]["x"].IOHandler);
    REQUIRE(prc.parent == getWritable(&o.iterations[1].particles["P"].particlePatches["extent"]));
    REQUIRE(o.iterations[1].particles["P"].particlePatches["extent"]["x"].parent == getWritable(&o.iterations[1].particles["P"].particlePatches["extent"]));
    prc.resetDataset(dset);
#else
    std::cerr << "Invasive tests not enabled. Hierarchy is not visible.\n";
#endif
}

TEST_CASE( "wrapper_test", "[core]" )
{
    Series o = Series("./new_openpmd_output.json", AccessType::CREATE);

    o.setOpenPMDextension(42);
    o.setIterationEncoding(IterationEncoding::fileBased);
    Series copy = o;
    REQUIRE(copy.openPMDextension() == 42);
    REQUIRE(copy.iterationEncoding() == IterationEncoding::fileBased);
    REQUIRE(copy.name() == "new_openpmd_output");
    copy.setOpenPMD("1.2.0");
    copy.setIterationEncoding(IterationEncoding::groupBased);
    copy.setName("other_name");
    REQUIRE(o.openPMD() == "1.2.0");
    REQUIRE(o.iterationEncoding() == IterationEncoding::groupBased);
    REQUIRE(o.name() == "other_name");

    o.iterations[1].meshes["E"]["x"].resetDataset(Dataset(Datatype::USHORT, {42}));
    MeshRecordComponent mrc = o.iterations[1].meshes["E"]["x"];
    REQUIRE(mrc.getDatatype() == Datatype::USHORT);
    REQUIRE(mrc.getExtent() == Extent{42});
    mrc.resetDataset(Dataset(Datatype::LONG_DOUBLE, {7}));
    REQUIRE(o.iterations[1].meshes["E"]["x"].getDatatype() == Datatype::LONG_DOUBLE);
    REQUIRE(o.iterations[1].meshes["E"]["x"].getExtent() == Extent{7});

    Container< Iteration, uint64_t > its = o.iterations;
    its[1].meshes["E"]["y"].resetDataset(Dataset(Datatype::CHAR, {2}));
    REQUIRE(o.iterations[1].meshes["E"].count("y") == 1);
    REQUIRE(o.iterations[1].meshes["E"]["y"].getDatatype() == Datatype::CHAR);
    REQUIRE(o.iterations[1].meshes["E"]["y"].getExtent() == Extent{2});
    o.iterations[1].meshes["E"]["z"].resetDataset(Dataset(Datatype::FLOAT, {1234}));
    REQUIRE(its[1].meshes["E"].count("z") == 1);
    REQUIRE(its[1].meshes["E"]["z"].getDatatype() == Datatype::FLOAT);
    REQUIRE(its[1].meshes["E"]["z"].getExtent() == Extent{1234});

    o.iterations[2];
    REQUIRE(its.count(2) == 1);
    its[3];
    REQUIRE(o.iterations.count(3) == 1);

    double value = 42.;
    o.iterations[4].meshes["E"]["y"].resetDataset(Dataset(Datatype::DOUBLE, {1}));
    o.iterations[4].meshes["E"]["y"].makeConstant(value);
    MeshRecordComponent mrc2 = o.iterations[4].meshes["E"]["y"];
#if openPMD_USE_INVASIVE_TESTS
    REQUIRE(*mrc2.m_isConstant);
#endif
    double loadData;
    mrc2.loadChunk(shareRaw(&loadData), {0}, {1});
    o.flush();
    REQUIRE(loadData == value);
    // TODO: do we want to be able to make data constant after already writing it once?
    // value = 43.;
    // mrc2.makeConstant(value);
    REQUIRE_THROWS_WITH(mrc2.makeConstant(value),
                        Catch::Equals("A recordComponent can not (yet) be made constant after it has been written."));
    std::array< double, 1 > moreData = {{ 112233. }};
    o.iterations[4].meshes["E"]["y"].loadChunk(shareRaw(moreData), {0}, {1});
    o.flush();
    REQUIRE(moreData[0] == value);
    auto all_data = o.iterations[4].meshes["E"]["y"].loadChunk<double>();
    o.flush();
    REQUIRE(all_data.get()[0] == value);
#if openPMD_USE_INVASIVE_TESTS
    REQUIRE(o.iterations[4].meshes["E"]["y"].m_chunks->empty());
    REQUIRE(mrc2.m_chunks->empty());
#endif

    MeshRecordComponent mrc3 = o.iterations[5].meshes["E"]["y"];
    o.iterations[5].meshes["E"]["y"].resetDataset(Dataset(Datatype::DOUBLE, {1}));
    int wrongData = 42;
    REQUIRE_THROWS_WITH(o.iterations[5].meshes["E"]["y"].storeChunk(shareRaw(&wrongData), {0}, {1}),
                        Catch::Equals("Datatypes of chunk data (INT) and record component (DOUBLE) do not match."));
    std::shared_ptr< double > storeData = std::make_shared< double >(44);
    o.iterations[5].meshes["E"]["y"].storeChunk(storeData, {0}, {1});
#if openPMD_USE_INVASIVE_TESTS
    REQUIRE(o.iterations[5].meshes["E"]["y"].m_chunks->size() == 1);
    REQUIRE(mrc3.m_chunks->size() == 1);
#endif
    o.flush();
#if openPMD_USE_INVASIVE_TESTS
    REQUIRE(o.iterations[5].meshes["E"]["y"].m_chunks->empty());
    REQUIRE(mrc3.m_chunks->empty());
#endif

    o.iterations[6].particles["electrons"].particlePatches["numParticles"][RecordComponent::SCALAR].resetDataset(Dataset(determineDatatype< uint64_t >(), {4}));
    auto dset = Dataset(Datatype::DOUBLE, {1});
    o.iterations[6].particles["electrons"]["position"][RecordComponent::SCALAR].resetDataset(dset);
    o.iterations[6].particles["electrons"]["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);
    ParticlePatches pp = o.iterations[6].particles["electrons"].particlePatches;
    REQUIRE(pp["numParticles"][RecordComponent::SCALAR].getDatatype() == determineDatatype< uint64_t >());
    REQUIRE(pp["numParticles"][RecordComponent::SCALAR].getExtent() == Extent{4});
    pp["prop"]["x"].resetDataset(Dataset(Datatype::DOUBLE, {7}));
    REQUIRE(o.iterations[6].particles["electrons"].particlePatches["prop"]["x"].getDatatype() == Datatype::DOUBLE);
    REQUIRE(o.iterations[6].particles["electrons"].particlePatches["prop"]["x"].getExtent() == Extent{7});
    size_t idx = 0;
    uint64_t val = 10;
#if openPMD_USE_INVASIVE_TESTS
    REQUIRE(o.iterations[6].particles["electrons"].particlePatches["numParticles"][RecordComponent::SCALAR].m_chunks->empty());
    REQUIRE(pp["numParticles"][RecordComponent::SCALAR].m_chunks->empty());
#endif
    pp["numParticles"][RecordComponent::SCALAR].store(idx, val);
#if openPMD_USE_INVASIVE_TESTS
    REQUIRE(o.iterations[6].particles["electrons"].particlePatches["numParticles"][RecordComponent::SCALAR].m_chunks->size() == 1);
    REQUIRE(pp["numParticles"][RecordComponent::SCALAR].m_chunks->size() == 1);
#endif
    std::stringstream u64str;
    u64str << determineDatatype<uint64_t>();
    REQUIRE_THROWS_WITH(o.iterations[6].particles["electrons"].particlePatches["numParticles"][RecordComponent::SCALAR].store(idx+1, 42.),
                        Catch::Equals("Datatypes of patch data (DOUBLE) and dataset (" + u64str.str() + ") do not match."));
    o.iterations[6].particles["electrons"].particlePatches["numParticles"][RecordComponent::SCALAR].store(idx+1, val+1);
#if openPMD_USE_INVASIVE_TESTS
    REQUIRE(o.iterations[6].particles["electrons"].particlePatches["numParticles"][RecordComponent::SCALAR].m_chunks->size() == 2);
    REQUIRE(pp["numParticles"][RecordComponent::SCALAR].m_chunks->size() == 2);
#endif
    o.flush();
#if openPMD_USE_INVASIVE_TESTS
    REQUIRE(o.iterations[6].particles["electrons"].particlePatches["numParticles"][RecordComponent::SCALAR].m_chunks->empty());
    REQUIRE(pp["numParticles"][RecordComponent::SCALAR].m_chunks->empty());
#endif
}

TEST_CASE( "use_count_test", "[core]" )
{
    Series o = Series("./new_openpmd_output.json", AccessType::CREATE);

    MeshRecordComponent mrc = o.iterations[1].meshes["E"]["x"];
    mrc.resetDataset(Dataset(determineDatatype<uint16_t>(), {42}));
    std::shared_ptr< uint16_t > storeData = std::make_shared< uint16_t >(44);
    REQUIRE(storeData.use_count() == 1);
    mrc.storeChunk(storeData, {0}, {1});
    REQUIRE(storeData.use_count() == 2);
    o.flush();
    REQUIRE(storeData.use_count() == 1);

#if openPMD_USE_INVASIVE_TESTS
    PatchRecordComponent pprc = o.iterations[6].particles["electrons"].particlePatches["numParticles"][RecordComponent::SCALAR];
    auto dset = Dataset(Datatype::DOUBLE, {1});
    o.iterations[6].particles["electrons"]["position"][RecordComponent::SCALAR].resetDataset(dset);
    o.iterations[6].particles["electrons"]["positionOffset"][RecordComponent::SCALAR].resetDataset(dset);
    pprc.resetDataset(Dataset(determineDatatype<uint64_t>(), {4}));
    pprc.store(0, static_cast< uint64_t >(1));
    REQUIRE(static_cast< Parameter< Operation::WRITE_DATASET >* >(pprc.m_chunks->front().parameter.get())->data.use_count() == 1);
#endif
}

TEST_CASE( "empty_record_test", "[core]" )
{
    Series o = Series("./new_openpmd_output.json", AccessType::CREATE);

    o.iterations[1].meshes["E"].setComment("No assumption about contained RecordComponents will be made");
    REQUIRE_THROWS_WITH(o.flush(),
                        Catch::Equals("A Record can not be written without any contained RecordComponents: E"));
    o.iterations[1].meshes["E"][RecordComponent::SCALAR].resetDataset(Dataset(Datatype::DOUBLE, {1}));
    o.flush();
}

TEST_CASE( "zero_extent_component", "[core]" )
{
    Series o = Series("./new_openpmd_output.json", AccessType::CREATE);

    auto E_x = o.iterations[1].meshes["E"]["x"];
    E_x.setComment("Datasets must contain dimensions.");
    REQUIRE_THROWS_WITH(E_x.resetDataset(Dataset(Datatype::LONG, {})),
                        Catch::Equals("Dataset extent must be at least 1D."));
    REQUIRE_THROWS_WITH(E_x.makeEmpty<int>(0),
                        Catch::Equals("Dataset extent must be at least 1D."));
    E_x.resetDataset(Dataset(Datatype::DOUBLE, {1}));
}

TEST_CASE( "no_file_ending", "[core]" )
{
    REQUIRE_THROWS_WITH(Series("./new_openpmd_output", AccessType::CREATE),
                        Catch::Equals("Unknown file format! Did you specify a file ending?"));
    REQUIRE_THROWS_WITH(Series("./new_openpmd_output_%T", AccessType::CREATE),
                        Catch::Equals("Unknown file format! Did you specify a file ending?"));
    REQUIRE_THROWS_WITH(Series("./new_openpmd_output_%05T", AccessType::CREATE),
                        Catch::Equals("Unknown file format! Did you specify a file ending?"));
}
