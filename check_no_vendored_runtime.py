#!/usr/bin/env python3
"""Fail if a built wheel vendors the MSVC C++ / OpenMP runtime DLLs.

delvewheel (cibuildwheel's default Windows wheel-repair tool since v4.0)
vendors, *by default*, the Visual C++ runtime (msvcp140.dll, ...) into
``<package>.libs/`` under a content-hashed (mangled) name. A private, mangled
copy of the C++ runtime collides with the system ``msvcp140.dll`` used by
OTHER C++ extension modules loaded in the same Python process (e.g. ImpactX,
WarpX): C++ objects / heap buffers allocated by one C runtime and freed or
filled by the other corrupt the CRT / heap state -> hard crash (segfault).
This was observed downstream when reading data via ``to_df()`` /
``get_attribute()`` right after the 0.17.1.post1 re-release.

The VC++ redistributable runtime is a *system* component -- Python itself
depends on it, and numpy / scipy / h5py never bundle it -- so it must NOT be
vendored into our wheels. This guard scans a wheel (or an installed package
directory) and exits non-zero if any such runtime DLL is bundled. Run it on
every Windows wheel after the repair step.

Note: this intentionally does NOT flag ``libstdc++`` / ``libc++`` on Linux /
macOS, where auditwheel / delocate bundling the C++ runtime is the accepted,
symbol-versioned manylinux/macOS behavior. The dangerous case is the Windows
C++ runtime specifically.

Usage:
    python check_no_vendored_runtime.py <wheel|dir> [<wheel|dir> ...]
"""
import fnmatch
import os
import sys
import zipfile

# Visual C++ / UCRT runtime DLLs that must come from the system, never bundled.
# Unversioned wildcards so a future toolset bump (msvcp150, new _N sidecars,
# ...) is still caught -- these prefixes are MSVC-reserved, nothing else uses
# them, so over-matching is not a concern.
_FORBIDDEN = (
    "msvcp*.dll", "vcruntime*.dll", "concrt*.dll", "vccorlib*.dll",
    "vcomp*.dll", "vcamp*.dll", "msvcr*.dll", "ucrtbase*.dll",
    "api-ms-win-*.dll",
)


def _is_forbidden(name):
    base = os.path.basename(name).lower()
    return any(fnmatch.fnmatch(base, pat) for pat in _FORBIDDEN)


def _dlls_in_wheel(path):
    with zipfile.ZipFile(path) as z:
        return [n for n in z.namelist() if n.lower().endswith(".dll")]


def _dlls_in_dir(path):
    out = []
    for root, _dirs, files in os.walk(path):
        for f in files:
            if f.lower().endswith(".dll"):
                out.append(os.path.join(root, f))
    return out


def check(path):
    if os.path.isdir(path):
        dlls = _dlls_in_dir(path)
    elif not os.path.exists(path):
        print("ERROR: %s does not exist." % path)
        return 2
    elif not os.path.isfile(path):
        print("ERROR: %s is not a wheel file or directory." % path)
        return 2
    elif zipfile.is_zipfile(path):
        dlls = _dlls_in_wheel(path)
    else:
        print("ERROR: %s is not a valid wheel/zip file." % path)
        return 2
    bad = sorted({d for d in dlls if _is_forbidden(d)})
    print("== %s: %d bundled DLL(s)" % (os.path.basename(path), len(dlls)))
    for d in sorted(dlls):
        print("     %s%s" % (d, "   <-- FORBIDDEN runtime" if _is_forbidden(d) else ""))
    if bad:
        names = ", ".join(os.path.basename(b) for b in bad)
        print("ERROR: %s vendors the VC++/UCRT runtime (%s).\n"
              "       Exclude it from delvewheel via "
              "CIBW_REPAIR_WHEEL_COMMAND_WINDOWS (--exclude)." %
              (os.path.basename(path), names))
        return 1
    return 0


def main(argv):
    if not argv:
        print("usage: check_no_vendored_runtime.py <wheel|dir> [...]")
        return 2
    rc = 0
    for p in argv:
        rc |= check(p)
    if rc == 0:
        print("OK: no VC++/UCRT runtime DLLs vendored.")
    return rc


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
