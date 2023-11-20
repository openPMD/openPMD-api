#!/usr/bin/env python3
#
# Copyright 2021-2023 Axel Huebl
#
# This file is part of openPMD-api.
#

# This file is a maintainer tool to bump the versions inside openPMD-api's
# source directory at all places where necessary.
#
import datetime
from pathlib import Path
import re
import sys

import requests

try:
    from configupdater import ConfigUpdater
except ImportError:
    print("Warning: Cannot update .ini files without 'configupdater'")
    print("Consider running 'python -m pip install configupdater'")
    ConfigUpdater = None
    sys.exit(1)


# Maintainer Inputs ###########################################################

print("""Hi there, this is an openPMD maintainer tool to update the source
code of openPMD-api to a new version.
For it to work, you need write access on the source directory and
you should be working in a clean git branch without ongoing
rebase/merge/conflict resolves and without unstaged changes.""")

# check source dir
REPO_DIR = Path(__file__).parent.parent.parent.absolute()
print(f"\nYour current source directory is: {REPO_DIR}")

REPLY = input("Are you sure you want to continue? [Y/n] ")
print()
if not REPLY in ["Y", "y", ""]:
    print("You did not confirm with 'y', aborting.")
    sys.exit(1)


# New Version #################################################################

# AMReX development HEAD
amrex_gh = requests.get('https://api.github.com/repos/AMReX-Codes/amrex/commits/development')
amrex_HEAD = amrex_gh.json()["sha"]

MAJOR = input("MAJOR version? (e.g., 1) ")
MINOR = input("MINOR version? (e.g., 0) ")
PATCH = input("PATCH version? (e.g., 0) ")
SUFFIX = input("SUFFIX? (e.g., dev) ")
if SUFFIX != "":
    SUFFIX_STR = f"-{SUFFIX}"

VERSION_STR_NOSUFFIX=f"{MAJOR}.{MINOR}{PATCH}"
VERSION_STR=f"{MAJOR}.{MINOR}.{PATCH}{SUFFIX_STR}"

echo
echo "Your new version is: ${VERSION_STR}"
echo

read -p "Is this information correct? Will now start updating! [y/N] " -r
echo


# Ask for new #################################################################

print("""We will now run a few sed commands on your source directory.
Please answer the following questions about the version number
you want to require from AMReX:\n""")

print(f"Currently, WarpX builds against this AMReX commit/branch/sha: {amrex_branch}")
print(f"AMReX HEAD commit (development branch): {amrex_HEAD}")
amrex_new_branch = input(f"Update AMReX commit/branch/sha: ").strip()
if not amrex_new_branch:
    amrex_new_branch = amrex_branch
    print(f"--> Nothing entered, will keep: {amrex_branch}")
print()

print(f"Currently, a pre-installed AMReX is required at least at version: {amrex_minimal}")
today = datetime.date.today().strftime("%y.%m")
amrex_new_minimal = input(f"New minimal AMReX version (e.g. {today})? ").strip()
if not amrex_new_minimal:
    amrex_new_minimal = amrex_minimal
    print(f"--> Nothing entered, will keep: {amrex_minimal}")

print()
print(f"New AMReX commit/branch/sha: {amrex_new_branch}")
print(f"New minimal AMReX version:   {amrex_new_minimal}\n")

REPLY = input("Is this information correct? Will now start updating! [y/N] ")
print()
if not REPLY in ["Y", "y"]:
    print("You did not confirm with 'y', aborting.")
    sys.exit(1)


# Updates #####################################################################

# run_test.sh (used also for Azure Pipelines)
run_test_path = str(REPO_DIR.joinpath("run_test.sh"))
with open(run_test_path, encoding='utf-8') as f:
    run_test_content = f.read()
    #   branch/commit/tag (git fetcher) version
    #     cd amrex && git checkout COMMIT_TAG_OR_BRANCH && cd -
    run_test_content = re.sub(
        r'(.*cd\s+amrex.+git checkout\s+--detach\s+)(.+)(\s+&&\s.*)',
        r'\g<1>{}\g<3>'.format(amrex_new_branch),
        run_test_content, flags = re.MULTILINE)

with open(run_test_path, "w", encoding='utf-8') as f:
    f.write(run_test_content)

# CI: legacy build check in .github/workflows/cuda.yml
ci_gnumake_path = str(REPO_DIR.joinpath(".github/workflows/cuda.yml"))
with open(ci_gnumake_path, encoding='utf-8') as f:
    ci_gnumake_content = f.read()
    #   branch/commit/tag (git fetcher) version
    #     cd ../amrex && git checkout COMMIT_TAG_OR_BRANCH && cd -
    ci_gnumake_content = re.sub(
        r'(.*cd\s+\.\./amrex.+git checkout\s+--detach\s+)(.+)(\s+&&\s.*)',
        r'\g<1>{}\g<3>'.format(amrex_new_branch),
        ci_gnumake_content, flags = re.MULTILINE)

with open(ci_gnumake_path, "w", encoding='utf-8') as f:
    f.write(ci_gnumake_content)

if ConfigUpdater is not None:
    # WarpX-tests.ini
    tests_ini_path = str(REPO_DIR.joinpath("Regression/WarpX-tests.ini"))
    cp = ConfigUpdater()
    cp.optionxform = str
    cp.read(tests_ini_path)
    cp['AMReX']['branch'].value = amrex_new_branch
    cp.update_file()

    # WarpX-GPU-tests.ini
    tests_gpu_ini_path = str(REPO_DIR.joinpath("Regression/WarpX-GPU-tests.ini"))
    cp = ConfigUpdater()
    cp.optionxform = str
    cp.read(tests_gpu_ini_path)
    cp['AMReX']['branch'].value = amrex_new_branch
    cp.update_file()

# WarpX references to AMReX: cmake/dependencies/AMReX.cmake
with open(amrex_cmake_path, encoding='utf-8') as f:
    amrex_cmake_content = f.read()

    #   branch/commit/tag (git fetcher) version
    #     set(WarpX_amrex_branch "development" ...
    amrex_cmake_content = re.sub(
        r'(.*set\(WarpX_amrex_branch\s+")(.+)("\s+.*)',
        r'\g<1>{}\g<3>'.format(amrex_new_branch),
        amrex_cmake_content, flags = re.MULTILINE)

    #   minimal (external) version
    #     find_package(AMReX YY.MM CONFIG ...
    amrex_cmake_content = re.sub(
        r'(.*find_package\(AMReX\s+)(.+)(\s+CONFIG\s+.*)',
        r'\g<1>{}\g<3>'.format(amrex_new_minimal),
        amrex_cmake_content, flags = re.MULTILINE)

with open(amrex_cmake_path, "w", encoding='utf-8') as f:
    f.write(amrex_cmake_content)


# Epilogue ####################################################################

print("""Done. Please check your source, e.g. via
  git diff
now and commit the changes if no errors occurred.""")
