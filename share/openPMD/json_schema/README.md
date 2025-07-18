# JSON Validation

This folder contains a JSON schema for validation of openPMD files written as `.json` files.

## Usage

### Generating the JSON schema

For improved readability, maintainability and documentation purposes, the JSON schema is written in `.toml` format and needs to be "compiled" to `.json` files first before usage.
To do this, the openPMD-api installs a tool named `openpmd-convert-toml-json` which can be used to convert between JSON and TOML files in both directions, e.g.:

```bash
openpmd-convert-toml-json @series.toml > series.json
```

A `Makefile` is provided in this folder to automate generating the needed JSON files from the TOML files.

### Verifying a file against the JSON schema

In theory, the JSON schema should be applicable by any JSON validator. This JSON schema is written in terms of multiple files however, and most validators require special care to properly set up the links between the single files. A Python script `check.py` is provided in this folder which sets up the [Python jsonschema](https://python-jsonschema.readthedocs.io) library and verifies a file against it, e.g.:

```bash
./check.py path/to/my/dataset.json
```

For further usage notes check the documentation of the script itself `./check.py --help`.

## Caveats

The openPMD standard is not entirely expressible in terms of a JSON schema:

* Many semantic dependencies, e.g., that the `position/x` and `position/y` vectors of a particle species need to be of the same size, or that the `axisLabels` have the same dimensionality as the dataset itself, will go unchecked.
* The `meshesPath` is assumed to be `meshes/` and the `particlesPath` is assumed to be `particles/`. This dependency cannot be expressed.

While a large part of the openPMD standard can indeed be verified by checking against a static JSON schema, the standard is generally large enough to make this approach come to its limits. Verification of a JSON schema is similar to the use of a naive recursive-descent parser. Error messages may become unexpectedly verbose and not very informative, especially when parsing disjunctive statements such as "A Record is either a scalar Record Component or a vector of non-scalar Record Components". We have taken care to decide disjunctive statements early on, e.g. with json-schema's support for `if` statements, but error messages may in general become unwieldy even due to tiny mistakes far down in the parse tree.

The layout of attributes is assumed to be that which is created by the JSON backend of the openPMD-api. Both the longhand and shorthand forms are recognized:

```json
"meshesPath": {
  "datatype": "STRING",
  "value": "meshes/"
},
"particlesPath": "particles/"
```

For a custom-written verification of openPMD datasets, also consider using the [openPMD-validator](https://github.com/openPMD/openPMD-validator).
