import numpy as np
import openpmd_api as io


def span_write(filename):
    series = io.Series(filename, io.Access_Type.create)

    datatype = np.dtype("double")
    length = 10
    extent = [length]
    dataset = io.Dataset(datatype, extent)

    # `Series.write_iterations()` and `Series.read_iterations()` are
    # intentionally restricted APIs that ensure a workflow which also works
    # in streaming setups, e.g. an iteration cannot be opened again once
    # it has been closed.
    # `Series.iterations` can be directly accessed in random-access workflows.
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

    # The files in 'series' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # When running out of scope on return, the 'Series' destructor is called.
    # Alternatively, one can call `series.close()` to the same effect as
    # calling the destructor, including the release of file handles.
    series.close()


if __name__ == "__main__":
    for ext in io.file_extensions:
        if ext == "sst" or ext == "ssc":
            continue
        span_write("../samples/span_write_python." + ext)
