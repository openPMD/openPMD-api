#!/usr/bin/env python3
#
# Copyright 2021-2023 Axel Huebl
#
# This file is part of openPMD-api.
#

# This file is a maintainer tool to bump the versions inside openPMD-api's
# source directory at all places where necessary.
#
from pathlib import Path
import re
import sys

# Maintainer Inputs ###########################################################

print(
    """Hi there, this is an openPMD maintainer tool to update the source
code of openPMD-api to a new version.
For it to work, you need write access on the source directory and
you should be working in a clean git branch without ongoing
rebase/merge/conflict resolves and without unstaged changes."""
)

# check source dir
# REPO_DIR = Path(__file__).parent.parent.parent.absolute()
REPO_DIR = Path(__file__).parent.absolute()
print(f"\nYour current source directory is: {REPO_DIR}")

REPLY = input("Are you sure you want to continue? [Y/n] ")
print()
if REPLY not in ["Y", "y", ""]:
    print("You did not confirm with 'y', aborting.")
    sys.exit(1)

MAJOR = input("MAJOR version? (e.g., 1) ")
MINOR = input("MINOR version? (e.g., 0) ")
PATCH = input("PATCH version? (e.g., 0) ")
SUFFIX = input("SUFFIX? (e.g., dev) ")

VERSION_STR = f"{MAJOR}.{MINOR}.{PATCH}"
VERSION_STR_SUFFIX = VERSION_STR + (f"-{SUFFIX}" if SUFFIX else "")

print()
print(f"Your new version is: {VERSION_STR_SUFFIX}")

with open(str(REPO_DIR.joinpath("README.md")), encoding="utf-8") as f:
    for line in f:
        match = re.search(r"find_package.*openPMD *([^ ]*) *CONFIG\).*", line)
        if match:
            OLD_VERSION_STR = match.group(1)
            break

print(f"The old version is: {OLD_VERSION_STR}")
print()


REPLY = input("Is this information correct? Will now start updating! [y/N] ")
print()
if REPLY not in ["Y", "y", ""]:
    print("You did not confirm with 'y', aborting.")
    sys.exit(1)


# Ask for new #################################################################

print(
    """We will now run a few sed commands on your source directory.
Please answer the following questions about the version number
you want to require from AMReX:\n"""
)

# Updates #####################################################################

# run_test.sh (used also for Azure Pipelines)
cmakelists_path = str(REPO_DIR.joinpath("CMakeLists.txt"))
with open(cmakelists_path, encoding="utf-8") as f:
    cmakelists_content = f.read()
    cmakelists_content = re.sub(
        r"^(project.*openPMD.*VERSION *)(.*)(\).*)$",
        r"\g<1>{}\g<3>".format(VERSION_STR),
        cmakelists_content,
        flags=re.MULTILINE,
    )

with open(cmakelists_path, "w", encoding="utf-8") as f:
    f.write(cmakelists_content)


def generic_replace(filename):
    filename = str(REPO_DIR.joinpath(filename))
    with open(filename, encoding="utf-8") as f:
        content = f.read()
        content = re.sub(re.escape(OLD_VERSION_STR), VERSION_STR, content)

    with open(filename, "w", encoding="utf-8") as f:
        f.write(content)


for file in ["docs/source/dev/linking.rst", "README.md"]:
    generic_replace(file)

version_hpp_path = str(REPO_DIR.joinpath("include/openPMD/version.hpp"))
with open(version_hpp_path, encoding="utf-8") as f:
    version_hpp_content = f.read()

    def replace(key, value):
        global version_hpp_content
        version_hpp_content = re.sub(
            r"^(#define OPENPMDAPI_VERSION_{}) .*$".format(re.escape(key)),
            r"\1 {}".format(value),
            version_hpp_content,
            flags=re.MULTILINE,
        )

    replace("MAJOR", MAJOR)
    replace("MINOR", MINOR)
    replace("PATCH", PATCH)
    replace("LABEL", '"{}"'.format(SUFFIX))

with open(version_hpp_path, "w", encoding="utf-8") as f:
    f.write(version_hpp_content)

# Epilogue ####################################################################

print(
    """Done. Please check your source, e.g. via
  git diff
now and commit the changes if no errors occurred."""
)
