#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2018-2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""

import openpmd_api as io

if __name__ == "__main__":
    series = io.Series(
        "../samples/git-sample/data%T.h5",
        io.Access.read_only,
        {"defer_iteration_parsing": True},
    )
    print(f"Read a Series with openPMD standard version {series.openPMD}")

    print(f"The Series contains {len(series.snapshots())} iterations:")
    for i in series.snapshots():
        print(f"\t {i}")
    print()

    # with defer_iteration_parsing, open() must be called explicitly
    i = series.snapshots()[100].open()
    print(f"Iteration 100 contains {len(i.meshes)} meshes:")
    for m in i.meshes:
        print(f"\t {m}")
    print()
    print(f"Iteration 100 contains {len(i.particles)} particle species:")
    for ps in i.particles:
        print(f"\t {ps}")
        print("With records:")
        for r in i.particles[ps]:
            print(f"\t {r}")

    # printing a scalar value
    electrons = i.particles["electrons"]
    charge = electrons["charge"]
    series.flush()
    print(f"And the first electron particle has a charge {charge[0]}")
    print()

    E_x = i.meshes["E"]["x"]
    shape = E_x.shape

    print(f"Field E.x has shape {shape} and datatype {E_x.dtype}")

    chunk_data = E_x[1:3, 1:3, 1:2]
    # print("Queued the loading of a single chunk from disk, "
    #       "ready to execute")
    series.flush()
    print("Chunk has been read from disk\nRead chunk contains:")
    print(chunk_data)
    # for row in range(2):
    #     for col in range(2):
    #         print("\t({0}|{1}|{2})\t{3}".format(
    #            row + 1, col + 1, 1, chunk_data[row*chunk_extent[1]+col])
    #         )
    #     print("")

    all_data = E_x.load_chunk()

    # The iteration can be closed in order to help free up resources.
    # The iteration's content will be flushed automatically.
    i.close()
    print(f"Full E/x is of shape {all_data.shape} and starts with:")
    print(all_data[0, 0, :5])

    # The files in 'series' are still open until the series is closed, at which
    # time it cleanly flushes and closes all open file handles.
    # One can close the object explicitly to trigger this.
    # Alternatively, this will automatically happen once the garbage collector
    # claims (every copy of) the series object.
    series.close()
