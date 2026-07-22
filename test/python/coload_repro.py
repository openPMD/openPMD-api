#!/usr/bin/env python3
#
# Copyright 2025 The openPMD Community
#
# License: LGPLv3+
#
# Reproduce/diagnose the co-load HDF5 fault seen in the ImpactX WASM wheel:
# openpmd_api and h5py each bundle their own static HDF5. Under Pyodide's
# single global namespace the two HDF5 copies interpose. Import ORDER matters
# (the first-loaded HDF5 becomes the primary), so we import openpmd_api first,
# run it, and only THEN load h5py -- mirroring ImpactX (impactx runs, then
# openpmd_api loads). Phase markers localize any fatal "memory access out of
# bounds" to a phase (mid-operation vs. teardown); the printed HDF5 versions
# tell an ABI mismatch (h5py bundling a different HDF5) from the same-version
# teardown fault.
#
# isort: skip_file  -- the openpmd_api-before-h5py import order is deliberate.
import numpy as np
import openpmd_api as io

print(
    f"[versions] openpmd_api {io.__version__}  variants={dict(io.variants)}", flush=True
)

data = np.arange(8, dtype=np.float64)

print("[1] openPMD write ...", flush=True)
series = io.Series("op.h5", io.Access.create)
mesh = series.iterations[0].meshes["E"]["x"]
mesh.reset_dataset(io.Dataset(data.dtype, data.shape))
mesh.store_chunk(data)
series.flush()
series.close()
del series, mesh

print("[2] openPMD read ...", flush=True)
series = io.Series("op.h5", io.Access.read_only)
back = series.iterations[0].meshes["E"]["x"].load_chunk()
series.flush()
series.close()
del series
assert np.array_equal(back, data), back

# load the SECOND, independently bundled HDF5 only now (openpmd_api is primary)
import h5py  # noqa: E402

print(
    f"[versions] h5py {h5py.__version__}  HDF5 {h5py.version.hdf5_version}", flush=True
)

print("[3] h5py write ...", flush=True)
with h5py.File("h5py.h5", "w") as f:
    f["x"] = data

print("[4] CO-LOAD ROUND-TRIP OK; now exiting -> teardown", flush=True)
