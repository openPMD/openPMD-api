# Copyright 2025 Franz Poeschel
#
# This file is part of openPMD-api.
#
# openPMD-api is free software: you can redistribute it and/or modify
# it under the terms of of either the GNU General Public License or
# the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# openPMD-api is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License and the GNU Lesser General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# and the GNU Lesser General Public License along with openPMD-api.
# If not, see <http://www.gnu.org/licenses/>.
#

import numpy as np
import openpmd_api as opmd

try:
    import hdf5plugin

    HAS_HDF5_PLUGIN = True
except ImportError:
    HAS_HDF5_PLUGIN = False


def write(filename, config):

    series = opmd.Series(
        f"../samples/compression_python/{filename}",
        opmd.Access.create_linear,
        config,
    )

    for i in range(10):
        current_iteration = series.snapshots()[i]

        # First, write an E mesh.
        E = current_iteration.meshes["E"]
        E.axis_labels = ["x", "y"]
        for dim in ["x", "y"]:
            component = E[dim]
            component.reset_dataset(opmd.Dataset(np.dtype("float"), [10, 10]))
            component[:, :] = np.reshape(
                np.arange(i * 100, (i + 1) * 100, dtype=np.dtype("float")),
                [10, 10],
            )

        # Now, write some e particles.
        e = current_iteration.particles["e"]
        for dim in ["x", "y"]:
            # Do not bother with a positionOffset
            position_offset = e["positionOffset"][dim]
            position_offset.reset_dataset(opmd.Dataset(np.dtype("int"), [100]))
            position_offset.make_constant(0)

            position = e["position"][dim]
            position.reset_dataset(opmd.Dataset(np.dtype("float"), [100]))
            position[:] = np.arange(
                i * 100, (i + 1) * 100, dtype=np.dtype("float")
            )


def main():

    # We start with two examples for ADIOS2.
    if "adios2" in opmd.variants and opmd.variants["adios2"]:
        simple_adios2_config = {
            # Backend can either be inferred from the filename ending, or
            # specified explicitly. In the latter case, the filename ending can
            # be given as a wildcard %E, openPMD will then pick a
            # default ending.
            "backend": "adios2",
            "adios2": {
                "dataset": {
                    # ADIOS2 supports adding multiple operators to a variable,
                    # hence we specify a list of operators here.
                    # How much sense this makes depends on the specific
                    # operators in use.
                    # If specifying only one operator, you can also replace the
                    # list by its only element as a shorthand
                    # (see next config example).
                    "operators": [
                        {
                            "type": "bzip2",
                            "parameters": {
                                # The available parameters depend
                                # on the operator.
                                # Here, we specify bzip2's compression level.
                                "clevel": 9
                            },
                        }
                    ]
                }
            },
        }
        write("adios2_with_bzip2.%E", simple_adios2_config)

        # The compression can also be specified per-dataset.
        # For more details, also check:
        # https://openpmd-api.readthedocs.io/en/latest/details/backendconfig.html#dataset-specific-configuration

        # This example will demonstrate the use of pattern matching.
        # adios2.dataset is now a list of dataset configurations. The specific
        # configuration to be used for a dataset will be determined by matching
        # the dataset name against the patterns specified by the 'select' key.
        # The actual configuration to be forwarded to the backend is stored
        # under the 'cfg' key.
        extended_adios2_config = {
            "backend": "adios2",
            "adios2": {
                "dataset": [
                    {
                        # This uses egrep-type regular expressions.
                        "select": "meshes/.*",
                        # Inside the cfg key, specify the actual config to
                        # be forwarded to the ADIOS2 dataset.
                        # So, specify the operators list again.
                        # Let's use Blosc for this.
                        "cfg": {
                            "operators": {
                                "parameters": {
                                    "clevel": 1,
                                    "doshuffle": "BLOSC_BITSHUFFLE",
                                },
                                "type": "blosc",
                            }
                        },
                    },
                    # Now, configure the particles.
                    {
                        # The match can either be against the path within the
                        # containing Iteration (e.g. 'meshes/E/x', as above)
                        # or (as in this example), against the full path
                        # (e.g. '/data/0/particles/e/position/x'). In this
                        # example, completely deactivate compression
                        # specifically for 'particles/e/position/x'.
                        # All other particle datasets will fall back to
                        # the default configuration specified below.
                        # Be careful when specifying compression per-Iteration.
                        # While this syntax fundamentally allows doing that,
                        # compressions once specified on an ADIOS2 variable
                        # will not be removed again. Since variable-encoding
                        # reuses ADIOS2 variables from previous Iterations,
                        # the compression configuration of the first Iteration
                        # will leak into all subsequent Iterations.
                        "select": "/data/[0-9]*/particles/e/position/x",
                        "cfg": {"operators": []},
                    },
                    # Now, the default configuration. In general, the dataset
                    # configurations are matched top-down, going for
                    # the first matching configuration. So, a default
                    # configuration could theoretically be specified
                    # by emplacing a catch-all pattern (regex: ".*") as the
                    # last option. However, we also define an explicit s
                    # horthand for specifying default configurations:
                    # Just omit the 'select' key. This special syntax
                    # is understood as the default configuration no matter
                    # where in the list it is emplaced, and it allows
                    # the backends to initialize the default configuration
                    # globally, instead of applying it selectively
                    # to each dataset that matches a catch-all pattern.
                    {
                        "cfg": {
                            "operators": {
                                "parameters": {"clevel": 2},
                                "type": "bzip2",
                            }
                        }
                    },
                ]
            },
        }
        write(
            "adios2_with_dataset_specific_configurations.%E",
            extended_adios2_config,
        )

    # Now, let's continue with HDF5.
    # HDF5 supports compression via so-called filters. These can be permanent
    # (applied to an entire dataset) and transient (applied to individual I/O
    # operations). The openPMD-api currently supports permanent filters. Refer
    # also to https://web.ics.purdue.edu/~aai/HDF5/html/Filters.html.

    # Filters are additionally distinguished by how tightly they integrate with
    # HDF5. The most tightly-integrated filter is Zlib, which has its own API
    # calls and hence also a special JSON/TOML configuration in openPMD:
    if "hdf5" in opmd.variants and opmd.variants["hdf5"]:
        hdf5_zlib_config = {
            "backend": "hdf5",
            "hdf5": {
                "dataset": {
                    "chunks": "auto",
                    "permanent_filters": {
                        "type": "zlib",  # mandatory parameter
                        "aggression": 5,  # optional, defaults to 1
                    }
                }
            },
        }
        write("hdf5_zlib.%E", hdf5_zlib_config)

        # All other filters have a common API and are identified by global IDs
        # registered with the HDF Group. More details can be found in the
        # H5Zpublic.h header. That header predefines a small number
        # of filter IDs.
        # These are directly supported by the openPMD-api: deflate, shuffle,
        # fletcher32, szip, nbit, scaleoffset.
        hdf5_predefined_filter_ids = {
            "backend": "hdf5",
            "hdf5": {
                "dataset": {
                    "chunks": "auto",
                    "permanent_filters": {
                        # mandatory parameter
                        "id": "fletcher32",
                        # optional parameter
                        "flags": "mandatory",
                        # optional parameter for filters identified by ID,
                        # mandatory only for zlib (see above)
                        "type": "by_id",
                    }
                }
            },
        }
        write("hdf5_predefined_filter_id.%E", hdf5_predefined_filter_ids)

        # Just like ADIOS2 with their operations, also HDF5 supports adding
        # multiple filters into a filter pipeline. The permanent_filters key
        # can hence also be given as a list.
        hdf5_filter_pipeline = {
            "backend": "hdf5",
            "hdf5": {
                "dataset": {
                    "chunks": "auto",
                    "permanent_filters": [
                        {"aggression": 5, "type": "zlib"},
                        {"flags": "mandatory", "id": "shuffle"},
                    ]
                }
            },
        }
        write("hdf5_filter_pipeline.%E", hdf5_filter_pipeline)

        # Dataset-specific backend configuration works independently from the
        # chosen backend and can hence also be used in HDF5. We will apply both
        # zlib and a fletcher32 filter, one to the meshes and one
        # to the particles.
        extended_hdf5_config = {
            "backend": "hdf5",
            "hdf5": {
                "dataset": [
                    {
                        "select": "meshes/.*",
                        "cfg": {
                            "chunks": "auto",
                            "permanent_filters": {
                                "type": "zlib",
                                "aggression": 5,
                            }
                        },
                    },
                    {
                        "select": "particles/.*",
                        "cfg": {
                            "chunks": "auto",
                            "permanent_filters": {
                                "id": "fletcher32",
                                "flags": "mandatory",
                            }
                        },
                    },
                ]
            },
        }
        write(
            "hdf5_with_dataset_specific_configurations.%E",
            extended_hdf5_config,
        )

    # For non-predefined IDs, the ID must be given as a number. This example
    # uses the Blosc2 filter with the permanent plugin ID 32026,
    # (defined in hdf5plugin.FILTERS["blosc2"]), available as part of Python's
    # hdf5plugin package. Generic filters referenced by ID can be configured
    # via the cd_values field. This field is an array of unsigned integers and
    # plugin-specific interpretation. For the Blosc2 plugin, indexes 0, 1, 2
    # and 3 are reserved. index 4 is the compression level, index 5 is a
    # boolean for activating shuffling and index 6 denotes
    # the compression method.

    if "hdf5" in opmd.variants and opmd.variants["hdf5"] and HAS_HDF5_PLUGIN:
        hdf5_blosc2_filter = {
            "backend": "hdf5",
            "hdf5": {
                "dataset": {
                    "chunks": "auto",
                    "permanent_filters": {
                        "cd_values": [0, 0, 0, 0, 4, 1, 5],
                        "flags": "mandatory",
                        "id": hdf5plugin.FILTERS["blosc2"],
                    },
                }
            },
        }

        write("hdf5_blosc_filter.%E", hdf5_blosc2_filter)


main()
