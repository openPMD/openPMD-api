#!/usr/bin/env python3
"""Functional smoke test: write a particle species + attribute, read it back.

This exercises the exact code path that crashed downstream (ImpactX) when the
0.17.1.post1 Windows wheel bundled a private MSVC C++ runtime: during
``Series.flush()`` a numpy buffer (allocated by numpy, i.e. the *system* C
runtime) is filled by the openPMD extension, and a particle-species attribute
(a ``std::string`` / ``double``) crosses the C++ <-> Python boundary via
``get_attribute()``. With a mismatched, vendored runtime this corrupts the
heap -> segfault. Importing numpy first co-loads a system-runtime C++
extension in the same process, mirroring the real-world usage.

A plain import + ``io.variants`` check (what the wheels were tested with
before) does NOT touch this path, which is why the regression shipped. This
round-trip does, and requires only numpy.

Usage:  python smoke_roundtrip.py
"""
import os
import sys
import tempfile

import numpy as np  # noqa: F401  -- also co-loads a system-runtime C++ extension
import openpmd_api as io


_BETA_REF = 0.987654321


def roundtrip(path):
    """Write a particle species + attribute, reopen, and read it back."""
    n = 32
    x = np.arange(n, dtype=np.float64)
    y = np.arange(n, dtype=np.float64) + 100.0

    # --- write ---
    series = io.Series(path, io.Access.create)
    beam = series.iterations[0].particles["beam"]
    for comp, arr in (("x", x), ("y", y)):
        rc = beam["position"][comp]
        rc.reset_dataset(io.Dataset(arr.dtype, list(arr.shape)))
        rc.store_chunk(arr)
    beam.set_attribute("beta_ref", _BETA_REF)
    series.flush()
    series.close()  # finalize before reopening the same file in this process

    # --- read back: the boundary-crossing path that crashed ---
    series = io.Series(path, io.Access.read_only)
    beam = series.iterations[0].particles["beam"]
    back_x = beam["position"]["x"].load_chunk()
    back_y = beam["position"]["y"].load_chunk()
    series.flush()  # fills the numpy buffers across the C++ <-> Python boundary
    got_beta = beam.get_attribute("beta_ref")
    series.close()

    assert np.array_equal(back_x, x), (back_x, x)
    assert np.array_equal(back_y, y), (back_y, y)
    assert abs(got_beta - _BETA_REF) < 1e-12, got_beta


def main():
    variants = dict(io.variants)
    print("openPMD", io.__version__, variants)

    # Exercise EVERY compiled-in backend. The ADIOS2 (.bp) round-trip also
    # proves ADIOS2 still works after we stopped vendoring the C++ runtime:
    # ADIOS2 and its deps are statically linked (no adios2/blosc2/... DLL is
    # bundled), so excluding msvcp140.dll does not affect them.
    targets = []
    if variants.get("hdf5"):
        targets.append("smoke.h5")
    if variants.get("adios2"):
        targets.append("smoke.bp")
    if not targets:  # header-only JSON is always available (e.g. WASM)
        targets.append("smoke.json")

    tmp = tempfile.mkdtemp(prefix="opmd-smoke-")
    for name in targets:
        path = os.path.join(tmp, name)
        roundtrip(path)
        print("openPMD smoke round-trip OK:", name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
