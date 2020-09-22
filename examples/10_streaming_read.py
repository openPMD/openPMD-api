#!/usr/bin/env python
import openpmd_api as io

run_streaming_example = False

if not run_streaming_example:
    exit(0)

if __name__ == "__main__":
    if 'adios2' not in io.variants or not io.variants['adios2']:
        print('This example requires ADIOS2')
        exit(0)

    series = io.Series("stream.sst", io.Access_Type.read_only)

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
