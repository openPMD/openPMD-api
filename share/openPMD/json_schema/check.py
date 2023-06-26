#!/usr/bin/env python

import argparse
import json
import os
from pathlib import Path
import sys

import jsonschema.validators


def parse_args(program_name):
    script_path = Path(os.path.dirname(os.path.realpath(sys.argv[0])))
    parser = argparse.ArgumentParser(
        # we need this for line breaks
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="""
Check JSON files against the openPMD JSON schema.

This tool validates an openPMD-formatted JSON file against the openPMD JSON
schema, using the jsonschema Python library as a backend.
Please use this script instead of the jsonschema directly since the openPMD
schema consists of several JSON files and this script ensures that
cross-referencing is set up correctly.

Note that the JSON schema is shipped in form of .toml files for ease
of reading, maintenance and documentation.
In order to perform a check, the .toml files need to be converted to .json
first.
The openPMD-api install a tool openpmd-convert-json-toml for this purpose.
Additionally, there is a Makefile shipped in the same folder as this Python
script which can be directly applied to generate the JSON schema.


Examples:
    {0} --help
    {0} --schema_root={1} <path/to/simData_000100.json>
""".format(os.path.basename(program_name), script_path / "series.json"))

    parser.add_argument(
        '--schema_root',
        default=script_path / 'series.json',
        help="""\
The .json file describing the root file of the schema to validate against.
"""
    )
    parser.add_argument('openpmd_file',
                        metavar='file',
                        nargs=1,
                        help="The file which to validate.")

    return parser.parse_args()


args = parse_args(sys.argv[0])

path = Path(os.path.dirname(os.path.realpath(args.schema_root)))
resolver = jsonschema.validators.RefResolver(
    base_uri=f"{path.as_uri()}/",
    referrer=True,
)

with open(args.openpmd_file[0], "r") as instance:
    jsonschema.validate(
        instance=json.load(instance),
        schema={"$ref": "./series.json"},
        resolver=resolver,
    )
    print("File {} was validated successfully against schema {}.".format(
        instance.name, args.schema_root))
