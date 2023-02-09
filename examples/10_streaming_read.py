#!/usr/bin/env python
import json
import sys

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

    series = io.Series("simData.sst", io.Access_Type.read_linear,
                       json.dumps(config))

    # Read all available iterations and print electron position data.
    # Use `series.read_iterations()` instead of `series.iterations`
    # for streaming support (while still retaining file-reading support).
    # Direct access to `series.iterations` is only necessary for random-access
    # of iterations. By using `series.read_iterations()`, the openPMD-api will
    # step through the iterations one by one, and going back to an iteration is
    # not possible once it has been closed.
    for iteration in series.read_iterations():
        print("Current iteration {}".format(iteration.iteration_index))
        electronPositions = iteration.particles["e"]["position"]
        loadedChunks = []
        shapes = []
        dimensions = ["x", "y", "z"]

        for i in range(3):
            dim = dimensions[i]
            rc = electronPositions[dim]
            loadedChunks.append(rc.load_chunk([0], rc.shape))
            shapes.append(rc.shape)

        # Closing the iteration loads all data and releases the current
        # streaming step.
        # If the iteration is not closed, it will be implicitly closed upon
        # opening the next iteration.
        iteration.close()

        # data is now available for printing
        for i in range(3):
            dim = dimensions[i]
            shape = shapes[i]
            print("dim: {}".format(dim))
            chunk = loadedChunks[i]
            print(chunk)

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # When running out of scope on return, the 'Series' destructor is called.
    # Alternatively, one can call `series.close()` to the same effect as
    # calling the destructor, including the release of file handles.
    series.close()
