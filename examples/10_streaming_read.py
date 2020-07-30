import openpmd_api as io
import numpy as np

if __name__ == "__main__":
    if not 'adios2' in io.variants or not io.variants['adios2']:
        print('This example requires ADIOS2')
        exit(0)

    options = """
    {
        "adios2": {
            "engine": {
                "type": "sst"
            }
        }
    }
"""
    series = io.Series("stream.bp", io.Access_Type.read_only, options)

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
