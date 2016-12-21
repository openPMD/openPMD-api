#define BOOST_TEST_MODULE libopenpmd_core_test
#include <boost/test/included/unit_test.hpp>

#include "../include/Output.hpp"

BOOST_AUTO_TEST_CASE(output_default_test)
{
    Output o = Output(IterationEncoding::fileBased);
    BOOST_TEST(o.name() == "new_openpmd_output");
    BOOST_TEST(o.basePath() == "/data/%T/");
    BOOST_TEST(o.meshesPath() == "meshes");
    BOOST_TEST(o.particlesPath() == "particles");
    BOOST_TEST(o.numAttributes() == 4);
    BOOST_TEST(o.iterations.size() == 0);
    BOOST_TEST(o.iterationEncoding() == IterationEncoding::fileBased);
}

BOOST_AUTO_TEST_CASE(output_constructor_test)
{
    Output o1 = Output(IterationEncoding::fileBased, "MyOutput");
    BOOST_TEST(o1.name() == "MyOutput");
    BOOST_TEST(o1.basePath() == "/data/%T/");
    BOOST_TEST(o1.meshesPath() == "meshes");
    BOOST_TEST(o1.particlesPath() == "particles");
    BOOST_TEST(o1.iterations.size() == 0);
    BOOST_TEST(o1.iterationEncoding() == IterationEncoding::fileBased);

    Output o2 = Output(IterationEncoding::fileBased,
               "MyCustomOutput",
               "customMeshesPath",
               "customParticlesPath");
    BOOST_TEST(o2.name() == "MyCustomOutput");
    BOOST_TEST(o2.basePath() == "/data/%T/");
    BOOST_TEST(o2.meshesPath() == "customMeshesPath");
    BOOST_TEST(o2.particlesPath() == "customParticlesPath");
    BOOST_TEST(o2.iterations.size() == 0);
    BOOST_TEST(o2.iterationEncoding() == IterationEncoding::fileBased);
}

BOOST_AUTO_TEST_CASE(output_modification_test)
{
    Output o = Output(IterationEncoding::fileBased);
    o.setIterationEncoding(IterationEncoding::groupBased);
    BOOST_TEST(o.iterationEncoding() == IterationEncoding::groupBased);
    o.setIterationEncoding(IterationEncoding::fileBased);
    BOOST_TEST(o.iterationEncoding() == IterationEncoding::fileBased);
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
    BOOST_TEST(i.time() == 0.f);
    BOOST_TEST(i.dt() == 0.f);
    BOOST_TEST(i.timeUnitSI() == 0.0);
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


BOOST_AUTO_TEST_CASE(record_default_test)
{

}