#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2018-2021 openPMD contributors
Authors: Axel Huebl, Fabian Koller
License: LGPLv3+
"""
import numpy as np
from openpmd_api import (Access, Dataset, Mesh_Record_Component, Series,
                         Unit_Dimension)

SCALAR = Mesh_Record_Component.SCALAR


if __name__ == "__main__":
    # open file for writing
    f = Series(
        "working/directory/2D_simData_py.h5",
        Access.create
    )

    # all required openPMD attributes will be set to reasonable default values
    # (all ones, all zeros, empty strings,...)
    # but one can also set customized values
    f.meshes_path = "custom_meshes_path"
    f.particles_path = "long_and_very_custom_particles_path"

    # it is possible to add and remove attributes
    f.comment = "This is fine and actually encouraged by the standard"
    f.set_attribute(
        "custom_attribute_name",
        "This attribute is manually added and can contain about any datatype "
        "you would want"
    )
    # note that removing attributes required by the standard typically makes
    # the file unusable for post-processing
    f.delete_attribute("custom_attribute_name")

    # attributes are read-write properties
    tmpItObj = f.iterations[1]
    tmpItObj.time = 42.0
    tmpItObj.dt = 1.0
    tmpItObj.time_unit_SI = 1.39e-16
    # everything that is accessed with [] should be interpreted as permanent
    # storage the objects sunk into these locations are deep copies
    f.iterations[2].comment = "This iteration will not appear in any output"
    del f.iterations[2]

    # this is a reference to an iteration
    reference = f.iterations[1]
    reference.comment = "Modifications to a reference will always be visible" \
                        " in the output"
    del reference

    # alternatively, a copy may be created and later re-assigned to
    # f.iterations[1]
    copy = f.iterations[1]  # TODO .copy()
    copy.comment = "Modifications to copies will only take effect after you " \
                   "reassign the copy"
    f.iterations[1] = copy
    del copy

    f.iterations[1].delete_attribute("comment")

    cur_it = f.iterations[1]

    # the underlying concept for numeric data is the openPMD Record
    # https://github.com/openPMD/openPMD-standard/blob/upcoming-1.0.1/STANDARD.md#scalar-vector-and-tensor-records
    # Meshes are specialized records
    cur_it.meshes["generic_2D_field"].unit_dimension = {
        Unit_Dimension.L: -3, Unit_Dimension.M: 1}

    # as this is a reference, it modifies the original resource
    lowRez = cur_it.meshes["generic_2D_field"]
    lowRez.grid_spacing = [1, 1]
    lowRez.grid_global_offset = [0, 600]

    # del cur_it.meshes["generic_2D_field"]
    cur_it.meshes["lowRez_2D_field"] = lowRez
    del lowRez

    # particles are handled very similar
    electrons = cur_it.particles["electrons"]
    electrons.set_attribute(
        "NoteWorthyParticleSpeciesProperty",
        "Observing this species was a blast.")
    electrons["displacement"].unit_dimension = {Unit_Dimension.M: 1}
    electrons["displacement"]["x"].unit_SI = 1.e-6
    del electrons["displacement"]
    electrons["weighting"][SCALAR].make_constant(1.e-5)

    mesh = cur_it.meshes["lowRez_2D_field"]
    mesh.axis_labels = ["x", "y"]

    # data is assumed to reside behind a pointer as a contiguous column-major
    # array shared data ownership during IO is indicated with a smart pointer
    partial_mesh = np.arange(5, dtype=np.double)

    # before storing record data, you must specify the dataset once per
    # component this describes the datatype and shape of data as it should be
    # written to disk
    d = Dataset(partial_mesh.dtype, extent=[2, 5])
    d.set_compression("zlib", 9)
    d.set_custom_transform("blosc:compressor=zlib,shuffle=bit,lvl=1;nometa")
    mesh["x"].reset_dataset(d)

    electrons = cur_it.particles["electrons"]

    mpiDims = [4]
    partial_particlePos = np.arange(2, dtype=np.float32)
    d = Dataset(partial_particlePos.dtype, extent=mpiDims)
    electrons["position"]["x"].reset_dataset(d)

    partial_particleOff = np.arange(2, dtype=np.uint)
    d = Dataset(partial_particleOff.dtype, mpiDims)
    electrons["positionOffset"]["x"].reset_dataset(d)

    dset = Dataset(np.dtype("uint64"), extent=[2])
    electrons.particle_patches["numParticles"][SCALAR].reset_dataset(dset)
    electrons.particle_patches["numParticlesOffset"][SCALAR]. \
        reset_dataset(dset)

    dset = Dataset(partial_particlePos.dtype, extent=[2])
    electrons.particle_patches["offset"].unit_dimension = \
        {Unit_Dimension.L: 1}
    electrons.particle_patches["offset"]["x"].reset_dataset(dset)
    electrons.particle_patches["extent"].unit_dimension = \
        {Unit_Dimension.L: 1}
    electrons.particle_patches["extent"]["x"].reset_dataset(dset)

    # at any point in time you may decide to dump already created output to
    # disk note that this will make some operations impossible (e.g. renaming
    # files)
    f.flush()

    # chunked writing of the final dataset at a time is supported
    # this loop writes one row at a time
    mesh_x = np.array([
        [1,  3,  5,  7,  9],
        [11, 13, 15, 17, 19]
    ])
    particle_position = np.array([0.1, 0.2, 0.3, 0.4], dtype=np.float32)
    particle_position_offset = [0, 1, 2, 3]
    for i in [0, 1]:
        for col in [0, 1, 2, 3, 4]:
            partial_mesh[col] = mesh_x[i, col]

        mesh["x"][i, 0:5] = partial_mesh
        # operations between store and flush MUST NOT modify the pointed-to
        # data
        f.flush()
        # after the flush completes successfully, access to the shared
        # resource is returned to the caller

        for idx in [0, 1]:
            partial_particlePos[idx] = particle_position[idx + 2*i]
            partial_particleOff[idx] = particle_position_offset[idx + 2*i]

        numParticlesOffset = 2*i
        numParticles = 2

        o = numParticlesOffset
        u = numParticles + o
        electrons["position"]["x"][o:u] = partial_particlePos
        electrons["positionOffset"]["x"][o:u] = partial_particleOff

        electrons.particle_patches["numParticles"][SCALAR].store(
            i, np.array([numParticles], dtype=np.uint64))
        electrons.particle_patches["numParticlesOffset"][SCALAR].store(
            i, np.array([numParticlesOffset], dtype=np.uint64))

        electrons.particle_patches["offset"]["x"].store(
            i,
            np.array([particle_position[numParticlesOffset]],
                     dtype=np.float32))
        electrons.particle_patches["extent"]["x"].store(
            i,
            np.array([
                particle_position[numParticlesOffset + numParticles - 1] -
                particle_position[numParticlesOffset]
            ], dtype=np.float32))

    mesh["y"].reset_dataset(d)
    mesh["y"].unit_SI = 4
    constant_value = 0.3183098861837907
    # for datasets that contain a single unique value, openPMD offers
    # constant records
    mesh["y"].make_constant(constant_value)

    # The files in 'f' are still open until the object is destroyed, on
    # which it cleanly flushes and closes all open file handles.
    # One can delete the object explicitly (or let it run out of scope) to
    # trigger this.
    del f
