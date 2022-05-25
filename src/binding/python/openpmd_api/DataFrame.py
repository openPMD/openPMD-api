"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import math

import numpy as np

try:
    import pandas as pd
    found_pandas = True
except ImportError:
    found_pandas = False


def particles_to_dataframe(particle_species, slice=None):
    """
    Load all records of a particle species into a Pandas DataFrame.

    Parameters
    ----------
    particle_species : openpmd_api.ParticleSpecies
        A ParticleSpecies class in openPMD-api.
    slice : np.s_, optional
        A numpy slice that can be used to load only a sub-selection of
        particles.

    Returns
    -------
    pandas.DataFrame
        A pandas dataframe with particles as index and openPMD record
        components of the particle_species as columns.

    Raises
    ------
    ImportError
        Raises an exception if pandas is not installed

    See Also
    --------
    numpy.s_ : the slice object to sub-select
    openpmd_api.BaseRecordComponent.available_chunks : available chunks that
        are optimal arguments for the slice parameter
    pandas.DataFrame : the central dataframe object created here
    """
    if not found_pandas:
        raise ImportError("pandas NOT found. Install pandas for DataFrame "
                          "support.")
    if slice is None:
        slice = np.s_[()]

    columns = {}

    for record_name, record in particle_species.items():
        for rc_name, rc in record.items():
            if record.scalar:
                column_name = record_name
            else:
                column_name = record_name + "_" + rc_name
            columns[column_name] = rc[slice]
            particle_species.series_flush()
            if not math.isclose(1.0, rc.unit_SI):
                columns[column_name] = np.multiply(
                    columns[column_name], rc.unit_SI)

    return pd.DataFrame(columns)
