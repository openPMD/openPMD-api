# JSON Validation

This folder contains a JSON schema for validation of openPMD files written as `.json` files.

## Usage

### Generating the JSON schema

For improved readability, maintainability and documentation purposes, the JSON schema is written in `.toml` format and needs to be "compiled" to `.json` files first before usage.
To do this, the openPMD-api installs a tool named `openpmd-convert-json-toml` which can be used to convert between JSON and TOML files in both directions, e.g.:

```bash
openpmd_convert-json-toml @series.toml > series.json
```

A `Makefile` is provided in this folder to simplify the application of this conversion tool.

### Verifying a file against the JSON schema

In theory, the JSON schema should be applicable by any JSON validator. This JSON schema is written in terms of multiple files however, and most validators require special care to properly set up the links between the single files. A Python script `check.py` is provided in this folder which sets up the [Python jsonschema](https://python-jsonschema.readthedocs.io) library and verifies a file against it, e.g.:

```bash
./check.py path/to/my/dataset.json
```

For further usage notes check the documentation of the script itself `./check.py --help`.

## Caveats

The openPMD standard is not entirely expressible in terms of a JSON schema:

* Many semantic dependencies, e.g. that the `position/x` and `position/y` vector of a particle species be of the same size, or that the `axisLabels` have the same dimensionality as the dataset itself, will go unchecked.
* The `meshesPath` is assumed to be `meshes/` and the `particlesPath` is assumed to be `particles/`. This dependency cannot be expressed.

While a large part of the openPMD standard can indeed be verified by checking against a JSON schema, the standard is generally large enough to make this approach come to its limits. Verification of a JSON schema is similar to the use of a naive recursive-descent parser. Error messages will often be unexpectedly verbose and not very informative.
A challenge for the JSON validator are disjunctive statements such as "A Record is either a scalar Record Component or a vector of non-scalar Record Components". If there is even a tiny mistake somewhere down in the hierarchy, the entire disjunctive branch will fail evaluating.

The layout of attributes is assumed to be that which is created by the JSON backend of the openPMD-api, e.g.:

```json
"meshesPath": {
  "datatype": "STRING",
  "value": "meshes/"
}
```

Support for an abbreviated notation such as `"meshesPath": "meshes/"` is currently not (yet) available.
