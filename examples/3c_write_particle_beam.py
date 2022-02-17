#!/usr/bin/env python3
"""
Authors: Wei-Hou Tan, Axel Huebl
Description: Generate a beam distribution and save it in the openPMD format.
Date: 2022
License: BSD-3-Clause-LBNL
"""
import numpy as np
from openpmd_api import (Access, Dataset, Record_Component, Series,
                         Unit_Dimension)

SCALAR = Record_Component.SCALAR


if __name__ == "__main__":
    f = Series("beam.h5", Access.create)

    # all required openPMD attributes will be set to reasonable default values
    # (all ones, all zeros, empty strings, ...)
    # but one can also set domain-specific values

    # constants
    qe = eV = 1.602176634e-19  # C or Joules, respectively
    me = 9.1093837015e-31      # kg
    c = 299792458.             # m/s

    # beam parameters
    energy_central = 500.0e6 * eV
    pz_central = energy_central / c  # ultrarelativistic limit
    print(pz_central)
    delta_pz = 1.e-9 * pz_central
    delta_px = 1.e-12 * pz_central
    sigma_x = 0.5e-6  # 0.5 micron
    sigma_z = 1.0e-6  #   1 micron
    charge = 1.e-12  # 1 pico Coulomb

    # numerics
    num = 1000  # number of macro-particles in the beam

    # we just need a single snapshot of the beam data
    cur_it = f.iterations[0]

    # particles
    electrons = cur_it.particles["beam"]
    electrons.set_attribute("comment",
        "What is love? Beam is love. Charged, focused love.")

    # define a charge
    #   note: if you specify beam.q_tot in the inputs file, then we will
    #         rescale the beam herein accordingly
    dset = Dataset(np.dtype("float64"), extent=[num])
    electrons["charge"][SCALAR].reset_dataset(dset)
    electrons["charge"][SCALAR].make_constant(qe)
    # quantity is "Current * Time" in ISQ
    electrons["charge"].unit_dimension = {Unit_Dimension.I: 1, Unit_Dimension.T: 1}
    electrons["charge"][SCALAR].unit_SI = -1.0  # data is already in Coulomb
    # the beam charge is equally distributed over all particles
    electrons["weighting"][SCALAR].reset_dataset(dset)
    electrons["weighting"][SCALAR].make_constant(charge / qe / num)  # positive

    # mass
    electrons["mass"][SCALAR].reset_dataset(dset)
    electrons["mass"][SCALAR].make_constant(me)
    electrons["mass"].unit_dimension = {Unit_Dimension.M: 1}
    electrons["mass"][SCALAR].unit_SI = 1.0  # data is already in kg

    # positions are split in fine (position) and coarse (positionOffset)
    # we set positionOffset to zero at just use position
    particlePos_x = np.random.normal(scale=sigma_x, size=num).astype(np.float64)
    particlePos_y = np.random.normal(scale=sigma_x, size=num).astype(np.float64)
    particlePos_z = np.random.normal(scale=sigma_z, size=num).astype(np.float64)

    d = Dataset(particlePos_x.dtype, extent=particlePos_x.shape)
    electrons["position"]["x"].reset_dataset(d)
    electrons["position"]["y"].reset_dataset(d)
    electrons["position"]["z"].reset_dataset(d)
    electrons["position"]["x"].store_chunk(particlePos_x)
    electrons["position"]["y"].store_chunk(particlePos_y)
    electrons["position"]["z"].store_chunk(particlePos_z)

    electrons["positionOffset"]["x"].reset_dataset(d)
    electrons["positionOffset"]["y"].reset_dataset(d)
    electrons["positionOffset"]["z"].reset_dataset(d)
    electrons["positionOffset"]["x"].make_constant(0.)
    electrons["positionOffset"]["y"].make_constant(0.)
    electrons["positionOffset"]["z"].make_constant(0.)

    electrons["position"].unit_dimension = {Unit_Dimension.L: 1}
    electrons["positionOffset"].unit_dimension = {Unit_Dimension.L: 1}

    # momentum
    particleMomentum_x = np.random.normal(0.0, delta_px, num).astype(np.float64)
    particleMomentum_y = np.random.normal(0.0, delta_px, num).astype(np.float64)
    particleMomentum_z = np.random.normal(pz_central, delta_pz, num).astype(np.float64)

    electrons["momentum"].unit_dimension = {Unit_Dimension.M: 1, Unit_Dimension.T: -1, Unit_Dimension.L: 1}

    d = Dataset(particleMomentum_x.dtype, extent=particleMomentum_x.shape)
    electrons["momentum"]["x"].reset_dataset(d)
    electrons["momentum"]["y"].reset_dataset(d)
    electrons["momentum"]["z"].reset_dataset(d)
    electrons["momentum"]["x"].store_chunk(particleMomentum_x)
    electrons["momentum"]["y"].store_chunk(particleMomentum_y)
    electrons["momentum"]["z"].store_chunk(particleMomentum_z)

    f.flush()

    # flush & close file
    del f
