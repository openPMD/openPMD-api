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

print("""Hi there, this is an openPMD maintainer tool to update the source
code of openPMD-api to a new version.
For it to work, you need write access on the source directory and
you should be working in a clean git branch without ongoing
rebase/merge/conflict resolves and without unstaged changes.""")

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
VERSION_STR_SUFFIX_WITH_DOT = VERSION_STR + (f".{SUFFIX}" if SUFFIX else "")

print()
print(f"Your new version is: {VERSION_STR_SUFFIX}")

# Recover the old version from the Readme.
# Do not use CMakeLists.txt as it might already contain the upcoming version
# code.
with open(str(REPO_DIR.joinpath("README.md")), encoding="utf-8") as f:
    for line in f:
        match = re.search(r"find_package.*openPMD *([^ ]*) *CONFIG\).*", line)
        if match:
            OLD_VERSION_STR_README = match.group(1)
            break

with open(str(REPO_DIR.joinpath("CMakeLists.txt")), encoding="utf-8") as f:
    for line in f:
        match = re.search(r"project\(openPMD *VERSION *(.*)\)", line)
        if match:
            OLD_VERSION_STR_CMAKE = match.group(1)
            break

OLD_VERSION_TAG = ""
with open(str(REPO_DIR.joinpath("include/openPMD/version.hpp")),
          encoding="utf-8") as f:
    for line in f:
        match = re.search(r'#define OPENPMDAPI_VERSION_LABEL "([^"]+)"', line)
        if match:
            OLD_VERSION_TAG = match.group(1)
            break

OLD_VERSION_SUFFIX = f"(-{OLD_VERSION_TAG})?" if OLD_VERSION_TAG else ""
# The order of the alternatives is important, since the Regex parser
# should greedily include the old version suffix
OLD_VERSION_STR = \
    f"({re.escape(OLD_VERSION_STR_CMAKE)}{OLD_VERSION_SUFFIX})" + \
    f"|({re.escape(OLD_VERSION_STR_README)})"

print(f"The old version is: {OLD_VERSION_STR}")
print()

REPLY = input("Is this information correct? Will now start updating! [y/N] ")
print()
if REPLY not in ["Y", "y", ""]:
    print("You did not confirm with 'y', aborting.")
    sys.exit(1)

# Ask for new #################################################################

print("""We will now run a few sed commands on your source directory.\n""")

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


def generic_replace(filename, previous, after):
    filename = str(REPO_DIR.joinpath(filename))
    with open(filename, encoding="utf-8") as f:
        content = f.read()
        content = re.sub(previous, after, content, flags=re.MULTILINE)

    with open(filename, "w", encoding="utf-8") as f:
        f.write(content)


for file in [
        "docs/source/dev/linking.rst",
        "README.md",
]:
    generic_replace(file, previous=OLD_VERSION_STR, after=VERSION_STR)

for file in ["CITATION.cff", "test/SerialIOTest.cpp"]:
    generic_replace(file, previous=OLD_VERSION_STR, after=VERSION_STR_SUFFIX)

generic_replace(
    "docs/source/index.rst",
    previous=r"``0.13.1-[^`]*``",
    after=f"``0.13.1-{VERSION_STR}``",
)
setup_py_path = str(REPO_DIR.joinpath("setup.py"))
with open(setup_py_path, encoding="utf-8") as f:
    for line in f:
        match = re.search(r"version='([^']+)',", line)
        if match:
            PREVIOUS_PIP_VERSION = match.group(1)
            break
generic_replace("setup.py",
                previous=PREVIOUS_PIP_VERSION,
                after=VERSION_STR_SUFFIX_WITH_DOT)
generic_replace(
    ".github/workflows/windows.yml",
    previous=f"{PREVIOUS_PIP_VERSION}0?",
    after=(f"{VERSION_STR_SUFFIX_WITH_DOT}0"
           if SUFFIX else VERSION_STR_SUFFIX_WITH_DOT),
)
generic_replace(
    "docs/source/conf.py",
    previous=r"^version.*=.*",
    after=f"version = u'{VERSION_STR}'",
)
generic_replace(
    "docs/source/conf.py",
    previous=r"^release.*=.*",
    after=f"release = u'{VERSION_STR_SUFFIX}'",
)

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

print("""Done. Please check your source, e.g. via
  git diff
now and commit the changes if no errors occurred.""")
