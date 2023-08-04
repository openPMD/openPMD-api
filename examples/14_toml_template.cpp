#include <openPMD/openPMD.hpp>

int main()
{
    std::string config = R"(
{
  "iteration_encoding": "variable_based",
  "toml": {
    "dataset": {"mode": "template"},
    "attribute": {"mode": "short"}
  }
}
)";

    openPMD::Series writeTemplate(
        "../samples/tomlTemplate.toml", openPMD::Access::CREATE, config);
    auto iteration = writeTemplate.writeIterations()[0];

    auto temperature =
        iteration.meshes["temperature"][openPMD::RecordComponent::SCALAR];
    temperature.resetDataset({openPMD::Datatype::FLOAT, {5, 5}});
}
