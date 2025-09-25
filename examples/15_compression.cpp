/* Copyright 2025 Franz Poeschel
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include <openPMD/openPMD.hpp>

#include <iostream>
#include <numeric>

void run_blosc2_filter_for_hdf5_example();

void write(std::string const &filename, std::string const &config)
{
    using namespace openPMD;
    std::cout << "Config for '" << filename << "' as JSON:\n"
              << json::merge(config, "{}") << "\n\n";
    Series series(
        "../samples/compression/" + filename, Access::CREATE_LINEAR, config);

    for (size_t i = 0; i < 10; ++i)
    {
        auto &current_iteration = series.snapshots()[i];

        // First, write an E mesh.
        auto &E = current_iteration.meshes["E"];
        E.setAxisLabels({"x", "y"});
        for (auto const &dim : {"x", "y"})
        {
            auto &component = E[dim];
            component.resetDataset({Datatype::FLOAT, {10, 10}});
            auto buffer_view =
                component.storeChunk<float>({0, 0}, {10, 10}).currentBuffer();
            // Now fill the prepared buffer with some nonsense data.
            std::iota(buffer_view.begin(), buffer_view.end(), i * 100);
        }

        // Now, write some e particles.
        auto &e = current_iteration.particles["e"];
        for (auto const &dim : {"x", "y"})
        {
            // Do not bother with a positionOffset
            auto &position_offset = e["positionOffset"][dim];
            position_offset.resetDataset({Datatype::INT, {100}});
            position_offset.makeConstant(0);

            auto &position = e["position"][dim];
            position.resetDataset({Datatype::FLOAT, {100}});
            auto buffer_view =
                position.storeChunk<float>({0}, {100}).currentBuffer();
            // Now fill the prepared buffer with some nonsense data.
            std::iota(buffer_view.begin(), buffer_view.end(), i * 100);
        }
    }
}

int main()
{
    // Backend specific configuration can be given in either JSON or TOML.
    // We will stick with TOML in this example, since it allows inline comments
    // and remains more legible for larger configurations.
    // If you are interested in the configurations as JSON, run the example and
    // their JSON equivalents will be printed to stdout.

#if openPMD_HAVE_ADIOS2
    // We start with two examples for ADIOS2.
    std::string const simple_adios2_config = R"(

        # Backend can either be inferred from the filename ending, or specified
        # explicitly. In the latter case, the filename ending can be given as
        # a wildcard %E, openPMD will then pick a default ending.
        backend = "adios2"

        # ADIOS2 supports adding multiple operators to a variable, hence we
        # specify a list of operators here (using TOML's double bracket syntax).
        # How much sense this makes depends on the specific operators in use.
        # If specifying only one operator, you can also replace the list by its
        # only element as a shorthand (see next config example).

        [[adios2.dataset.operators]]
        type = "bzip2"
        parameters.clevel = 9  # The available parameters depend
                               # on the operator.
                               # Here, we specify bzip2's compression level.
    )";
    write("adios2_with_bzip2.%E", simple_adios2_config);

    // The compression can also be specified per-dataset.
    // For more details, also check:
    // https://openpmd-api.readthedocs.io/en/latest/details/backendconfig.html#dataset-specific-configuration

    // This example will demonstrate the use of pattern matching.
    // adios2.dataset is now a list of dataset configurations. The specific
    // configuration to be used for a dataset will be determined by matching
    // the dataset name against the patterns specified by the 'select' key.
    // The actual configuration to be forwarded to the backend is stored under
    // the 'cfg' key.
    std::string const extended_adios2_config = R"(
        backend = "adios2"

        [[adios2.dataset]]
        # This uses egrep-type regular expressions.
        select = "meshes/.*"
        # Inside the cfg key, specify the actual config to be forwarded to the
        # ADIOS2 dataset.
        # So, specify the operators list again. Let's use Blosc for this.
        [adios2.dataset.cfg.operators]
        type = "blosc"
        parameters.doshuffle = "BLOSC_BITSHUFFLE"
        parameters.clevel = 1

        # Now, configure the particles.
        [[adios2.dataset]]
        # The match can either be against the path within the containing
        # Iteration (e.g. 'meshes/E/x', as above) or (as in this example),
        # against the full path (e.g. '/data/0/particles/e/position/x').
        # In this example, completely deactivate compression specifically for
        # 'particles/e/position/x'. All other particle datasets will
        # fall back to the default configuration specified below.
        # Be careful when specifying compression per-Iteration. While this
        # syntax fundamentally allows doing that, compressions once specified
        # on an ADIOS2 variable will not be removed again.
        # Since variable-encoding reuses ADIOS2 variables from previous
        # Iterations, the compression configuration of the first Iteration will
        # leak into all subsequent Iterations.
        select = "/data/[0-9]*/particles/e/position/x"
        cfg.operators = []

        # Now, the default configuration.
        # In general, the dataset configurations are matched top-down, going for
        # the first matching configuration. So, a default configuration could
        # theoretically be specified by emplacing a catch-all pattern
        # (regex: ".*") as the last option.
        # However, we also define an explicit shorthand for specifying default
        # configurations: Just omit the 'select' key. This special syntax is
        # understood as the default configuration no matter where in the list it
        # is emplaced, and it allows the backends to initialize the default
        # configuration globally, instead of applying it selectively to each
        # dataset that matches a catch-all pattern.
        [[adios2.dataset]]
        [adios2.dataset.cfg.operators]
        type = "bzip2"
        parameters.clevel = 2
    )";
    write(
        "adios2_with_dataset_specific_configurations.%E",
        extended_adios2_config);
#endif // openPMD_HAVE_ADIOS2

#if openPMD_HAVE_HDF5
    // Now, let's continue with HDF5.
    // HDF5 supports compression via so-called filters. These can be permanent
    // (applied to an entire dataset) and transient (applied to individual I/O
    // operations). The openPMD-api currently supports permanent filters. Refer
    // also to https://web.ics.purdue.edu/~aai/HDF5/html/Filters.html.

    // Filters are additionally distinguished by how tightly they integrate with
    // HDF5. The most tightly-integrated filter is Zlib, which has its own API
    // calls and hence also a special JSON/TOML configuration in openPMD:

    std::string const hdf5_zlib_config = R"(
        backend = "hdf5"

        [hdf5.dataset]
        chunks = "auto"

        [hdf5.dataset.permanent_filters]
        type = "zlib"   # mandatory parameter
        aggression = 5  # optional, defaults to 1
    )";
    write("hdf5_zlib.%E", hdf5_zlib_config);

    // All other filters have a common API and are identified by global IDs
    // registered with the HDF Group. More details can be found in the
    // H5Zpublic.h header. That header predefines a small number of filter IDs.
    // These are directly supported by the openPMD-api: deflate, shuffle,
    // fletcher32, szip, nbit, scaleoffset.

    std::string const hdf5_predefined_filter_ids = R"(
        backend = "hdf5"

        [hdf5.dataset]
        chunks = "auto"

        [hdf5.dataset.permanent_filters]
        id = "fletcher32"  # mandatory parameter
        # A filter can be applied as mandatory (execution should abort if the
        # filter cannot be applied) or as optional (execution should ignore when
        # the filter cannot be applied).
        flags = "mandatory"  # optional parameter
        type = "by_id"       # optional parameter for filters identified by ID,
                             # mandatory only for zlib (see above)
    )";
    write("hdf5_predefined_filter_id.%E", hdf5_predefined_filter_ids);

    // Just like ADIOS2 with their operations, also HDF5 supports adding
    // multiple filters into a filter pipeline. The permanent_filters key can
    // hence also be given as a list.

    std::string const hdf5_filter_pipeline = R"(
        backend = "hdf5"

        [hdf5.dataset]
        chunks = "auto"

        # pipeline consisting of two filters

        [[hdf5.dataset.permanent_filters]]
        type = "zlib"
        aggression = 5

        [[hdf5.dataset.permanent_filters]]
        id = "shuffle"
        flags = "mandatory"
    )";
    write("hdf5_filter_pipeline.%E", hdf5_filter_pipeline);

    // Dataset-specific backend configuration works independently from the
    // chosen backend and can hence also be used in HDF5. We will apply both a
    // zlib and a fletcher32 filter, one to the meshes and one to the particles.
    std::string const extended_hdf5_config = R"(
        backend = "hdf5"

        [[hdf5.dataset]]
        select = "meshes/.*"

        [hdf5.dataset.cfg]
        chunks = "auto"
        [hdf5.dataset.cfg.permanent_filters]
        type = "zlib"
        aggression = 5

        # Now, configure the particles.
        [[hdf5.dataset]]
        select = "particles/.*"

        [hdf5.dataset.cfg]
        chunks = "auto"
        [hdf5.dataset.cfg.permanent_filters]
        id = "fletcher32"
        flags = "mandatory"
    )";
    write("hdf5_with_dataset_specific_configurations.%E", extended_hdf5_config);

    // The following example runs the Blosc2 plugin which must be separately
    // installed. One simple way is to install the Python package hdf5plugin
    // which contains precompiled filters and then point HDF5_PLUGIN_PATH toward
    // the plugins directory therein (containing libh5blosc2.so). This example
    // assumes such a setup.
    if (getenv("HDF5_PLUGIN_PATH"))
    {
        // For non-predefined IDs, the ID must be given as a number. This
        // example uses the Blosc2 filter available from
        // https://pypi.org/project/hdf5plugin/,
        // with the permanent plugin ID 32026.
        // Generic filters referenced by ID can be configured via the cd_values
        // field. This field is an array of unsigned integers and
        // plugin-specific interpretation. For the Blosc2 plugin, indexes 0, 1,
        // 2 and 3 are reserved. index 4 is the compression level, index 5 is a
        // boolean for activating shuffling and index 6 denotes the compression
        // method. Compression method 5 is BLOSC_ZSTD.
        std::string hdf5_blosc_filter = R"(
            backend = "hdf5"

            [hdf5.dataset]
            chunks = "auto"

            [hdf5.dataset.permanent_filters]
            id = 32026
            flags = "mandatory"
            cd_values = [0, 0, 0, 0, 4, 1, 5]
        )";
        write("hdf5_blosc_filter.%E", hdf5_blosc_filter);
    }
#endif // openPMD_HAVE_HDF5
}
