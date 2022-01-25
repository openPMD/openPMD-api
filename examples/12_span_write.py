import numpy as np
import openpmd_api as io


def span_write(filename):
    series = io.Series(filename, io.Access_Type.create)

    datatype = np.dtype("double")
    length = 10
    extent = [length]
    dataset = io.Dataset(datatype, extent)

    iterations = series.write_iterations()
    for i in range(12):
        iteration = iterations[i]
        electronPositions = iteration.particles["e"]["position"]

        j = 0
        for dim in ["x", "y", "z"]:
            pos = electronPositions[dim]
            pos.reset_dataset(dataset)
            # The Python span API does not expose the extended version that
            # allows overriding the fallback buffer allocation
            span = pos.store_chunk([0], extent).current_buffer()
            for k in range(len(span)):
                span[k] = 3 * i * length + j * length + k
            j += 1
        iteration.close()


if __name__ == "__main__":
    for ext in io.file_extensions:
        if ext == "sst" or ext == "ssc":
            continue
        span_write("../samples/span_write_python." + ext)
