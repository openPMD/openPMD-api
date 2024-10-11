#include <openPMD/openPMD.hpp>

#include <algorithm>
#include <iostream>
#include <memory>
#include <numeric> // std::iota

using std::cout;
using namespace openPMD;

int main()
{
    if (!getVariants()["adios2"])
    {
        // Example configuration below selects the ADIOS2 backend
        return 0;
    }

    using position_t = double;
    /*
     * This example demonstrates how to use JSON/TOML-based dynamic
     * configuration for openPMD.
     * The following configuration is passed to the constructor of the Series
     * class and specifies the defaults to used for that Series.
     * This configuration can later be overridden as needed on a per-dataset
     * level.
     */
    std::string const defaults = R"END(
# This configuration is TOML-based
# JSON can be used alternatively, the openPMD-api will automatically detect
# the language being used
#
# Alternatively, the location of a JSON/TOML-file on the filesystem can
# be passed by adding an at-sign `@` in front of the path
# The format will then be recognized by filename extension, i.e. .json or .toml

backend = "adios2"
iteration_encoding = "group_based"
# The following is only relevant in read mode
defer_iteration_parsing = true

[adios2.engine]
type = "bp4"

# ADIOS2 allows adding several operators
# Lists are given in TOML by using double brackets
[[adios2.dataset.operators]]
type = "zlib"

parameters.clevel = 5
# Alternatively:
# [adios2.dataset.operators.parameters]
# clevel = 9

# For adding a further parameter:
# [[adios2.dataset.operators]]
# type = "some other parameter"
# # ...

[hdf5.dataset]
chunks = "auto"
)END";

    // open file for writing
    Series series =
        Series("../samples/dynamicConfig.bp", Access::CREATE, defaults);

    Datatype datatype = determineDatatype<position_t>();
    constexpr unsigned long length = 10ul;
    Extent global_extent = {length};
    Dataset dataset = Dataset(datatype, global_extent);
    std::shared_ptr<position_t> local_data(
        new position_t[length], [](position_t const *ptr) { delete[] ptr; });

    // `Series::writeIterations()` and `Series::readIterations()` are
    // intentionally restricted APIs that ensure a workflow which also works
    // in streaming setups, e.g. an iteration cannot be opened again once
    // it has been closed.
    // `Series::iterations` can be directly accessed in random-access workflows.
    WriteIterations iterations = series.writeIterations();
    for (size_t i = 0; i < 100; ++i)
    {
        Iteration iteration = iterations[i];
        Record electronPositions = iteration.particles["e"]["position"];

        std::iota(local_data.get(), local_data.get() + length, i * length);
        for (auto const &dim : {"x", "y", "z"})
        {
            RecordComponent pos = electronPositions[dim];
            pos.resetDataset(dataset);
            pos.storeChunk(local_data, Offset{0}, global_extent);
        }

        /*
         * We want different compression settings for this dataset, so we pass
         * a dataset-specific configuration.
         * Also showcase how to define an resizable dataset.
         * This time in JSON.
         */
        std::string const differentCompressionSettings = R"END(
{
  "resizable": true,
  "adios1": {
    "dataset": {
      "transform": "blosc:compressor=zlib,shuffle=bit,lvl=1;nometa"
    }
  },
  "adios2": {
    "dataset": {
      "operators": [
        {
          "type": "zlib",
          "parameters": {
            "clevel": 9
          }
        }
      ]
    }
  }
})END";
        Dataset differentlyCompressedDataset{Datatype::INT, {10}};
        differentlyCompressedDataset.options = differentCompressionSettings;

        auto someMesh = iteration.meshes["differentCompressionSettings"];
        someMesh.resetDataset(differentlyCompressedDataset);
        std::vector<int> dataVec(10, i);
        someMesh.storeChunk(dataVec, {0}, {10});

        iteration.close();
    }

    /* The files in 'series' are still open until the object is destroyed, on
     * which it cleanly flushes and closes all open file handles.
     * When running out of scope on return, the 'Series' destructor is called.
     * Alternatively, one can call `series.close()` to the same effect as
     * calling the destructor, including the release of file handles.
     */
    series.close();

    return 0;
}
