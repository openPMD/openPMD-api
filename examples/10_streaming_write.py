#!/usr/bin/env python
import json
import sys

import numpy as np
import openpmd_api as io

# pass-through for ADIOS2 engine parameters
# https://adios2.readthedocs.io/en/latest/engines/engines.html
config = {'adios2': {'engine': {}, 'dataset': {}}}
config['adios2']['engine'] = {'parameters': {'Threads': '4'}}
config['adios2']['dataset'] = {'operators': [{'type': 'bzip2'}]}

if __name__ == "__main__":
    # this block is for our CI, SST engine is not present on all systems
    backends = io.file_extensions
    if "sst" not in backends:
        print("SST engine not available in ADIOS2.")
        sys.exit(0)

    # create a series and specify some global metadata
    # change the file extension to .json, .h5 or .bp for regular file writing
    series = io.Series("simData.sst", io.Access_Type.create,
                       json.dumps(config))
    series.set_author("Franz Poeschel <f.poeschel@hzdr.de>")
    series.set_software("openPMD-api-python-examples")

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

        temperature = iteration.meshes["temperature"]
        temperature.unit_dimension = {io.Unit_Dimension.theta: 1.0}
        temperature.axis_labels = ["x", "y"]
        temperature.grid_spacing = [1., 1.]
        # temperature has no x,y,z components, so skip the last layer:
        temperature_dataset = temperature[io.Mesh_Record_Component.SCALAR]
        # let's say we are in a 3x3 mesh
        temperature_dataset.reset_dataset(
            io.Dataset(np.dtype("double"), [3, 3]))
        # temperature is constant
        temperature_dataset.make_constant(273.15)

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
