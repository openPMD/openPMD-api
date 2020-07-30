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
    series = io.Series("stream.bp", io.Access_Type.create, options)
    datatype = np.dtype("double")
    length = 10
    global_extent = [10]
    dataset = io.Dataset(datatype, global_extent)

    iterations = series.writeIterations()
    for i in range(100):
        iteration = iterations[i]
        electronPositions = iteration.particles["e"]["position"]

        local_data = np.arange(i * length, (i + 1) * length, dtype=datatype)
        for dim in ["x", "y", "z"]:
            pos = electronPositions[dim]
            pos.reset_dataset(dataset)
            pos[()] = local_data
        iteration.close()