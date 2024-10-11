#!/usr/bin/env python
import json

import numpy as np
import openpmd_api as io

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
    if not io.variants['adios2']:
        # Example configuration below selects the ADIOS2 backend
        return

    # create a series and specify some global metadata
    # change the file extension to .json, .h5 or .bp for regular file writing
    series = io.Series("../samples/dynamicConfig.bp", io.Access_Type.create,
                       defaults)

    # now, write a number of iterations (or: snapshots, time steps)
    for i in range(10):
        # Direct access to iterations is possible via `series.iterations`.
        # For streaming support, `series.write_iterations()` needs to be used
        # instead of `series.iterations`.
        # `Series.write_iterations()` and `Series.read_iterations()` are
        # intentionally restricted APIs that ensure a workflow which also works
        # in streaming setups, e.g. an iteration cannot be opened again once
        # it has been closed.
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
        local_data = np.arange(i * length, (i + 1) * length,
                               dtype=np.dtype("double"))
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
            'resizable': True,
            'adios2': {
                'dataset': {
                    'operators': []
                }
            },
            'adios1': {
                'dataset': {}
            }
        }
        config['adios2']['dataset'] = {
            'operators': [{
                'type': 'zlib',
                'parameters': {
                    'clevel': 9
                }
            }]
        }
        config['adios1']['dataset'] = {
            'transform': 'blosc:compressor=zlib,shuffle=bit,lvl=1;nometa'
        }

        temperature = iteration.meshes["temperature"]
        temperature.unit_dimension = {io.Unit_Dimension.theta: 1.0}
        temperature.axis_labels = ["x", "y"]
        temperature.grid_spacing = [1., 1.]
        # temperature has no x,y,z components, so skip the last layer:
        temperature_dataset = temperature
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

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # When running out of scope on return, the 'Series' destructor is called.
    # Alternatively, one can call `series.close()` to the same effect as
    # calling the destructor, including the release of file handles.
    series.close()


if __name__ == "__main__":
    main()
