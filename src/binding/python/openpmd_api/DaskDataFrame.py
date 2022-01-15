"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl, Dmitry Ganyushin, John Kirkham
License: LGPLv3+
"""
import numpy as np

try:
    import dask.dataframe as dd
    from dask.delayed import delayed
    found_dask = True
except ImportError:
    found_dask = False
try:
    import pandas  # noqa
    found_pandas = True
except ImportError:
    found_pandas = False


def read_chunk_to_df(species, chunk):
    stride = np.s_[chunk.offset[0]:chunk.offset[0]+chunk.extent[0]]
    return species.to_df(stride)


def particles_to_daskdataframe(particle_species):
    """
    Load all records of a particle species into a Dask DataFrame.

    Parameters
    ----------
    particle_species : openpmd_api.ParticleSpecies
        A ParticleSpecies class in openPMD-api.

    Returns
    -------
    dask.dataframe
        A dask dataframe with particles as index and openPMD record
        components of the particle_species as columns.

    Raises
    ------
    ImportError
        Raises an exception if dask or pandas are not installed

    See Also
    --------
    openpmd_api.BaseRecordComponent.available_chunks : available chunks that
        are used internally to parallelize particle processing
    dask.dataframe : the central dataframe object created here
    """
    if not found_dask:
        raise ImportError("dask NOT found. Install dask for Dask DataFrame "
                          "support.")
    if not found_pandas:  # catch this early: before delayed functions
        raise ImportError("pandas NOT found. Install pandas for DataFrame "
                          "support.")

    # get optimal chunks: query first non-constant record component and
    #                     assume the same chunking applies for all of them
    #                     in a particle species
    chunks = None
    for k_r, r in particle_species.items():
        for k_rc, rc in r.items():
            if not rc.constant:
                chunks = rc.available_chunks()
                break
        if chunks:
            break

    # only constant record components:
    # fall back to a single, big chunk here
    if chunks is None:
        for k_r, r in particle_species.items():
            for k_rc, rc in r.items():
                chunks = rc.available_chunks()
                break
            if chunks:
                break

    # merge DataFrames
    dfs = [
        delayed(read_chunk_to_df)(particle_species, chunk) for chunk in chunks
    ]
    df = dd.from_delayed(dfs)

    return df
