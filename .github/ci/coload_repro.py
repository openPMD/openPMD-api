#!/usr/bin/env python3
#
# Copyright 2025 The openPMD Community
#
# License: LGPLv3+
#
# Reproduce the co-load HDF5 teardown fault seen in the ImpactX WASM wheel:
# openpmd_api and h5py each bundle their own static HDF5. Under Pyodide's single
# global namespace the two HDF5 copies interpose (symbol visibility has no effect
# there), so their type registries tangle; at interpreter teardown openPMD's HDF5
# handler-destructor H5Tclose then faults ("not a datatype" -> wasm OOB), even
# though the round-trip itself succeeds.
#
# Exits 0 on a clean teardown; a nonzero exit / "memory access out of bounds"
# means the fault is still present.
import h5py  # a second, independently bundled HDF5
import numpy as np
import openpmd_api as io

# exercise openpmd_api's HDF5 (its handler builds the custom types that fault)
series = io.Series("op.h5", io.Access.create)
mesh = series.iterations[0].meshes["E"]["x"]
data = np.arange(8, dtype=np.float64)
mesh.reset_dataset(io.Dataset(data.dtype, data.shape))
mesh.store_chunk(data)
series.flush()
series.close()
del series, mesh

series = io.Series("op.h5", io.Access.read_only)
back = series.iterations[0].meshes["E"]["x"].load_chunk()
series.flush()
series.close()
del series
assert np.array_equal(back, data), back

# exercise h5py's HDF5 (the co-loaded second copy)
with h5py.File("h5py.h5", "w") as f:
    f["x"] = data

print("CO-LOAD ROUND-TRIP OK (openpmd_api + h5py); now exiting -> teardown")
