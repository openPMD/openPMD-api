#include <openPMD/auxiliary/TemplateFile.hpp>
#include <openPMD/openPMD.hpp>

std::string backendEnding()
{
    auto extensions = openPMD::getFileExtensions();
    if (auto it = std::find(extensions.begin(), extensions.end(), "toml");
        it != extensions.end())
    {
        return *it;
    }
    else
    {
        // Fallback for buggy old NVidia compiler
        return "json";
    }
}

void write()
{
    std::string config = R"(
{
  "iteration_encoding": "variable_based",
  "json": {
    "dataset": {"mode": "template"},
    "attribute": {"mode": "short"}
  },
  "toml": {
    "dataset": {"mode": "template"},
    "attribute": {"mode": "short"}
  }
}
)";

    openPMD::Series writeTemplate(
        "../samples/tomlTemplate." + backendEnding(),
        openPMD::Access::CREATE,
        config);
    auto iteration = writeTemplate.writeIterations()[0];

    openPMD::Dataset ds{openPMD::Datatype::FLOAT, {5, 5}};

    auto temperature =
        iteration.meshes["temperature"][openPMD::RecordComponent::SCALAR];
    temperature.resetDataset(ds);

    auto E = iteration.meshes["E"];
    E["x"].resetDataset(ds);
    E["y"].resetDataset(ds);
    /*
     * Don't specify datatype and extent for this one to indicate that this
     * information is not yet known.
     */
    E["z"].resetDataset({openPMD::Datatype::UNDEFINED});

    ds.extent = {10};

    auto electrons = iteration.particles["e"];
    electrons["position"]["x"].resetDataset(ds);
    electrons["position"]["y"].resetDataset(ds);
    electrons["position"]["z"].resetDataset(ds);

    electrons["positionOffset"]["x"].resetDataset(ds);
    electrons["positionOffset"]["y"].resetDataset(ds);
    electrons["positionOffset"]["z"].resetDataset(ds);
    electrons["positionOffset"]["x"].makeConstant(3.14);
    electrons["positionOffset"]["y"].makeConstant(3.14);
    electrons["positionOffset"]["z"].makeConstant(3.14);

    ds.dtype = openPMD::determineDatatype<uint64_t>();
    electrons.particlePatches["numParticles"][openPMD::RecordComponent::SCALAR]
        .resetDataset(ds);
    electrons
        .particlePatches["numParticlesOffset"][openPMD::RecordComponent::SCALAR]
        .resetDataset(ds);
    electrons.particlePatches["offset"]["x"].resetDataset(ds);
    electrons.particlePatches["offset"]["y"].resetDataset(ds);
    electrons.particlePatches["offset"]["z"].resetDataset(ds);
    electrons.particlePatches["extent"]["x"].resetDataset(ds);
    electrons.particlePatches["extent"]["y"].resetDataset(ds);
    electrons.particlePatches["extent"]["z"].resetDataset(ds);
}

void read()
{
    /*
     * The config is entirely optional, these things are also detected
     * automatically when reading
     */

    // std::string config = R"(
    // {
    //   "iteration_encoding": "variable_based",
    //   "toml": {
    //     "dataset": {"mode": "template"},
    //     "attribute": {"mode": "short"}
    //   }
    // }
    // )";

    openPMD::Series read(
        "../samples/tomlTemplate." + backendEnding(),
        openPMD::Access::READ_LINEAR);
    read.readIterations(); // @todo change to read.parseBase()

    std::string jsonConfig = R"(
{
  "iteration_encoding": "variable_based",
  "json": {
    "mode": "template"
  }
}
)";
    openPMD::Series cloned(
        "../samples/jsonTemplate.json", openPMD::Access::CREATE, jsonConfig);
    openPMD::auxiliary::initializeFromTemplate(cloned, read, 0);
    // Have to define the dataset for E/z as it is not defined in the template
    // @todo check that the dataset is defined only upon destruction, not at
    // flushing already
    cloned.writeIterations()[0].meshes["E"]["z"].resetDataset(
        {openPMD::Datatype::INT});
}

int main()
{
    write();
    read();
}
