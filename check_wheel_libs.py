#!/usr/bin/env python3
"""Pre-import smoke test: list the shared-library dependencies of openPMD's
compiled extension module(s) and flag the non-system ones.

Run this BEFORE importing openpmd_api in CI. If a non-system library was not
bundled into the wheel, you see exactly which one here -- instead of a bare
"DLL load failed" / "cannot open shared object file" from the import. The
extension is located via its spec (no import), so this still works when the
import itself would fail.

Usage:
    python check_wheel_libs.py [--strict]                 # installed package
    python check_wheel_libs.py [--strict] <file|dir|.whl> # wheel/binary/dir

    --strict   exit non-zero if any non-system dependency is not found
               (default: diagnostic only, exit 0, so CI continues to the
               import test and we can see what happens afterwards)
"""
import importlib.machinery
import importlib.util
import os
import sys
import tempfile
import zipfile

import list_library_deps as lld

PACKAGE = "openpmd_api"
_EXT_SUFFIXES = tuple(importlib.machinery.EXTENSION_SUFFIXES) + (
    ".pyd", ".so", ".dylib")


def _find_in_dir(d):
    found = []
    for root, _dirs, files in os.walk(d):
        for f in files:
            if f.endswith(_EXT_SUFFIXES):
                found.append(os.path.join(root, f))
    return found


def _installed_package_dirs():
    # find_spec locates the package WITHOUT executing its __init__ (which would
    # trigger the very import we are trying to diagnose).
    try:
        spec = importlib.util.find_spec(PACKAGE)
    except Exception:
        spec = None
    if spec is None:
        return []
    if spec.submodule_search_locations:
        return list(spec.submodule_search_locations)
    if spec.origin:
        return [os.path.dirname(spec.origin)]
    return []


def collect_binaries(arg):
    if arg is None:
        bins = []
        for d in _installed_package_dirs():
            bins += _find_in_dir(d)
        return bins
    if os.path.isdir(arg):
        return _find_in_dir(arg)
    if arg.endswith((".whl", ".zip")) and zipfile.is_zipfile(arg):
        tmp = tempfile.mkdtemp(prefix="wheel-libs-")
        with zipfile.ZipFile(arg) as z:
            z.extractall(tmp)
        return _find_in_dir(tmp)
    return [arg]  # a single binary


def main(argv):
    strict = "--strict" in argv
    positional = [a for a in argv if not a.startswith("--")]
    arg = positional[0] if positional else None
    bins = collect_binaries(arg)
    if not bins:
        print("check_wheel_libs: no compiled extension modules found "
              "(package %r not importable-spec / no binaries in %r)"
              % (PACKAGE, arg))
        return 0
    print("=== shared-library dependencies of %d extension module(s) ===\n"
          % len(bins))
    total_missing = 0
    for b in sorted(bins):
        _external, missing = lld.report(b)
        total_missing += missing
        print()
    if total_missing:
        print("WARNING: %d dependency/-ies were not found on the build host; "
              "if they are also absent at runtime the import will fail."
              % total_missing)
        if strict:
            print("(--strict) aborting with non-zero exit")
            return 1
    # diagnostic only by default -- the import test is the actual pass/fail gate
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
