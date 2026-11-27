"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl, Dmitry Ganyushin, John Kirkham
License: LGPLv3+
"""

import numpy as np

from .DataFrame import _floating_dtype


def read_chunk_to_df(species, chunk, attributes=None, floating=None):
    stride = np.s_[chunk.offset[0] : chunk.offset[0] + chunk.extent[0]]
    return species.to_df(attributes=attributes, slice=stride, floating=floating)


def particles_to_daskdataframe(particle_species, attributes=None, *, floating=None):
    """
    Load all records of a particle species into a Dask DataFrame.

    Parameters
    ----------
    particle_species : openpmd_api.ParticleSpecies
        A ParticleSpecies class in openPMD-api.
    attributes : list of strings, optional
        A list of attributes of the particle_species that should be read and
        added as extra columns.
    floating : numpy.dtype or type or string, optional
        The floating-point type of all floating-point columns, see
        openpmd_api.ParticleSpecies.to_df. Conversions are part of the delayed
        read of each chunk. By default, the stored type is kept.

    Returns
    -------
    dask.dataframe
        A dask dataframe with particles as rows and openPMD record
        components of the particle_species as columns. The row index restarts
        for each chunk. To index by particle id, use .set_index("id"), which
        reads all chunks and shuffles particles between partitions.

    Raises
    ------
    ImportError
        Raises an exception if dask or pandas are not installed
    TypeError
        Raises an exception if floating is not a floating-point type

    See Also
    --------
    openpmd_api.BaseRecordComponent.available_chunks : available chunks that
        are used internally to parallelize particle processing
    dask.dataframe : the central dataframe object created here
    """
    # import here for lazy imports
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

    if not found_dask:
        raise ImportError("dask NOT found. Install dask for Dask DataFrame support.")
    if not found_pandas:  # catch this early: before delayed functions
        raise ImportError("pandas NOT found. Install pandas for DataFrame support.")

    # catch invalid arguments now and not when the delayed reads are computed
    floating = _floating_dtype(floating)

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
        delayed(read_chunk_to_df)(
            particle_species, chunk=chunk, attributes=attributes, floating=floating
        )
        for chunk in chunks
    ]
    df = dd.from_delayed(dfs)

    # set a header for the first column (row index)
    #   note: this is NOT the particle id
    # TODO both these do not work:
    #   https://github.com/dask/dask/issues/10440
    # df.index.name = "row"
    # df.index = df.index.rename("row")

    return df
