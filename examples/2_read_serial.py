#!/usr/bin/env python
"""
This file is part of the openPMD-api.

Copyright 2018-2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import openpmd_api as io
import pickle


if __name__ == "__main__":
    series = io.Series("../samples/git-sample/data%T.h5",
                       io.Access.read_only)
    print("Read a Series with openPMD standard version %s" %
          series.openPMD)

    i = series.iterations[400]

    # Get a mesh.
    E = i.meshes["E"]
    #self.assertIsInstance(E, io.Mesh)
    E_x = E["x"]
    #self.assertIsInstance(E, io.Mesh_Record_Component)

    # Get a particle species.
    electrons = i.particles["electrons"]
    #self.assertIsInstance(electrons, io.ParticleSpecies)
    pos_y = electrons["position"]["y"]
    w = electrons["weighting"][io.Record_Component.SCALAR]

    # Pickle
    pickled_E_x = pickle.dumps(E_x)
    pickled_pos_y = pickle.dumps(pos_y)
    pickled_w = pickle.dumps(w)
    print(f"This is my pickled object:\n{pickled_E_x}\n")

    E_x = None
    del E_x

    # Unpickling the object
    unpickled_E_x = pickle.loads(pickled_E_x)
    unpickled_pos_y = pickle.loads(pickled_pos_y)
    unpickled_w = pickle.loads(pickled_w)
    print(
        f"This is a_dict of the unpickled object:\n{unpickled_E_x.position}\n")
    data = unpickled_E_x[()]
    unpickled_E_x.series_flush()
    print(data)

    data_pos_y = unpickled_pos_y[()]
    data_w = unpickled_w[()]
    unpickled_pos_y.series_flush()
    print(data_pos_y)
    print(data_w)
