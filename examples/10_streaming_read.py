#!/usr/bin/env python
import sys

import openpmd_api as io

if __name__ == "__main__":
    if 'adios2' not in io.variants or not io.variants['adios2']:
        print('This example requires ADIOS2')
        sys.exit(0)

    series = io.Series("stream.sst", io.Access_Type.read_only)

    backends = io.file_extensions
    if "sst" not in backends:
        print("SST engine not available in ADIOS2.")
        sys.exit(0)

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
        iteration.close()

        for i in range(3):
            dim = dimensions[i]
            shape = shapes[i]
            print("dim: {}".format(dim))
            chunk = loadedChunks[i]
            print(chunk)
