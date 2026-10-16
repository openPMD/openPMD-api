#!/usr/bin/env bash
#
# Copyright 2026 The openPMD Community
#
# License: LGPLv3+
#
# Verify that the test suite did not write any data files outside of the
# samples directory.
#
# The tests are run by CTest with the build's runtime output directory as the
# working directory (usually <build>/bin), and are expected to write all of
# their output below ../samples (i.e. <build>/samples). Run this script after
# CTest to detect any output that ended up somewhere else. This keeps the build
# directory clean and prevents tests from relying on files in the working
# directory.
#
# Usage: check_test_output.sh [runtime-output-dir]
#
# @result 0 if no stray test output was found, else 1

set -eu -o pipefail

# Working directory of the tests; defaults to the usual CMake location.
runtime_dir=${1:-build/bin}

if [ ! -d "${runtime_dir}" ]; then
    echo "# '${runtime_dir}' does not exist, skipping the output location check."
    exit 0
fi

# File endings used by the openPMD backends and their configuration files.
# Some backends (e.g. ADIOS2 BP) write a directory instead of a file, so both
# files and directories are matched.
extensions=(
    h5 hdf5
    bp bp3 bp4 bp5
    sst ssc
    json toml
)

ok=0
for extension in "${extensions[@]}"; do
    while IFS= read -r -d '' entry; do
        # Everything below a samples/ directory is expected.
        case "${entry}" in
            samples | samples/* | */samples | */samples/*) continue ;;
        esac
        echo "# Test output outside of samples/: ${entry}"
        ok=1
    done < <(find "${runtime_dir}" \
        \( -type f -o -type d \) -name "*.${extension}" -print0)
done

if [ ${ok} -ne 0 ]; then
    echo "#"
    echo "# SUMMARY"
    echo "# -------"
    echo "# The test suite must only write its output below ../samples."
    echo "# Move the files reported above into the samples directory."
else
    echo "All test output was written below samples/. OK."
fi

exit ${ok}
