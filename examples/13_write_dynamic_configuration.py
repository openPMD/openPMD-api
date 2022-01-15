#!/usr/bin/env python
import json
import openpmd_api as io
import numpy as np


# This example demonstrates how to use JSON/TOML-based dynamic
# configuration for openPMD.
# The following configuration is passed to the constructor of the Series
# class and specifies the defaults to used for that Series.
# This configuration can later be overridden as needed on a per-dataset
# level.

defaults = """
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

[adios1.dataset]
transform = "blosc:compressor=zlib,shuffle=bit,lvl=5;nometa"

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
"""


def main():
    if not io.variants["adios2"]:
        # Example configuration below selects the ADIOS2 backend
        return

    # create a series and specify some global metadata
    # change the file extension to .json, .h5 or .bp for regular file writing
    series = io.Series("../samples/dynamicConfig.bp", io.Access_Type.create, defaults)

    # now, write a number of iterations (or: snapshots, time steps)
    for i in range(10):
        # Use `series.write_iterations()` instead of `series.iterations`
        # for streaming support (while still retaining file-writing support).
        # Direct access to `series.iterations` is only necessary for
        # random-access of iterations. By using `series.write_iterations()`,
        # the openPMD-api will adhere to streaming semantics while writing.
        # In particular, this means that only one iteration can be written at a
        # time and an iteration can no longer be modified after closing it.
        iteration = series.write_iterations()[i]

        #######################
        # write electron data #
        #######################

        electronPositions = iteration.particles["e"]["position"]

        # openPMD attribute
        # (this one would also be set automatically for positions)
        electronPositions.unit_dimension = {io.Unit_Dimension.L: 1.0}
        # custom attribute
        electronPositions.set_attribute("comment", "I'm a comment")

        length = 10
        local_data = np.arange(i * length, (i + 1) * length, dtype=np.dtype("double"))
        for dim in ["x", "y", "z"]:
            pos = electronPositions[dim]
            pos.reset_dataset(io.Dataset(local_data.dtype, [length]))
            pos[()] = local_data

        # optionally: flush now to clear buffers
        iteration.series_flush()  # this is a shortcut for `series.flush()`

        ###############################
        # write some temperature data #
        ###############################

        # we want different compression settings here,
        # so we override the defaults
        # let's use JSON this time
        config = {
            "resizable": True,
            "adios2": {"dataset": {"operators": []}},
            "adios1": {"dataset": {}},
        }
        config["adios2"]["dataset"] = {
            "operators": [{"type": "zlib", "parameters": {"clevel": 9}}]
        }
        config["adios1"]["dataset"] = {
            "transform": "blosc:compressor=zlib,shuffle=bit,lvl=1;nometa"
        }

        temperature = iteration.meshes["temperature"]
        temperature.unit_dimension = {io.Unit_Dimension.theta: 1.0}
        temperature.axis_labels = ["x", "y"]
        temperature.grid_spacing = [1.0, 1.0]
        # temperature has no x,y,z components, so skip the last layer:
        temperature_dataset = temperature[io.Mesh_Record_Component.SCALAR]
        # let's say we are in a 3x3 mesh
        dataset = io.Dataset(np.dtype("double"), [3, 3])
        dataset.options = json.dumps(config)
        temperature_dataset.reset_dataset(dataset)
        # temperature is constant
        local_data = np.arange(i * 9, (i + 1) * 9, dtype=np.dtype("double"))
        local_data = local_data.reshape([3, 3])
        temperature_dataset[()] = local_data

        # After closing the iteration, the readers can see the iteration.
        # It can no longer be modified.
        # If not closing an iteration explicitly, it will be implicitly closed
        # upon creating the next iteration.
        iteration.close()


if __name__ == "__main__":
    main()
