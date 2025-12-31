"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import math

import numpy as np


def particles_to_dataframe(particle_species,
                           *legacy_args,
                           attributes=None,
                           slice=None):
    """
    Load all records of a particle species into a Pandas DataFrame.

    Parameters
    ----------
    particle_species : openpmd_api.ParticleSpecies
        A ParticleSpecies class in openPMD-api.
    legacy_args : tuple
        DO NOT USE. Catch-all for legacy, unnamed arguments.
    attributes : list of strings, optional
        A list of attributes of the particle_species that should be read and
        added as extra columns.
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
    # backwards compatibility: in openPMD-api 0.17+, we added the
    # additional "attributes" argument and moved slice= to the end.
    if legacy_args:
        if attributes is None and slice is None and len(legacy_args) == 1:
            slice = legacy_args[0]
            import warnings
            warnings.warn("The to_df() argument order changed in "
                          "openPMD-api 0.17.0!\nThe slice "
                          "argument must be passed as a named argument.",
                          DeprecationWarning
                          )
        else:
            raise RuntimeError("to_df() does not support unnamed arguments!")

    # import pandas here for a lazy import
    try:
        import pandas as pd
        found_pandas = True
    except ImportError:
        found_pandas = False

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

    df = pd.DataFrame(columns)

    if attributes is not None:
        for attribute in attributes:
            df[attribute] = particle_species.get_attribute(attribute)

    # set a header for the first column (row index)
    #   note: this is NOT the particle id
    df.index.name = "row"

    return df


def iterations_to_dataframe(series, species_name, attributes=None):
    """
    Load all iterations of a particle species into a Pandas DataFrame.

    Parameters
    ----------
    series : openpmd_api.Series
        A Series class in openPMD-api.
    species_name : string
        The name of a particle species.
    attributes : list of strings, optional
        A list of attributes of the particle_species that should be read and
        added as extra columns.

    Returns
    -------
    pandas.DataFrame
        A pandas dataframe with particles as index and openPMD record
        components of the particle_species as columns. Particles might be
        repeated over multiple iterations and an "iteration" column is
        added.

    Raises
    ------
    ImportError
        Raises an exception if pandas is not installed

    See Also
    --------
    pandas.DataFrame : the central dataframe object created here
    """
    # import pandas here for a lazy import
    try:
        import pandas as pd
    except ImportError:
        raise ImportError("pandas NOT found. Install pandas for DataFrame "
                          "support.")

    df = pd.concat(
        (
            iteration
            .particles[species_name]
            .to_df(attributes=attributes)
            .assign(iteration=i)
            for i, iteration in series.snapshots().items()
        ),
        axis=0,
        ignore_index=True,
    )

    return df


def iterations_to_cudf(series, species_name, attributes=None):
    """
    Load all iterations of a particle species into a cuDF DataFrame.

    Parameters
    ----------
    series : openpmd_api.Series
        A Series class in openPMD-api.
    species_name : string
        The name of a particle species.
    attributes : list of strings, optional
        A list of attributes of the particle_species that should be read and
        added as extra columns.

    Returns
    -------
    cudf.DataFrame
        A cuDF (RAPIDS) dataframe with particles as index and openPMD record
        components of the particle_species as columns. Particles might be
        repeated over multiple iterations and an "iteration" column is
        added.

    Raises
    ------
    ImportError
        Raises an exception if cuDF (RAPIDS) is not installed

    See Also
    --------
    cudf.DataFrame : the central dataframe object created here
    """
    # import pandas here for a lazy import
    try:
        import pandas  # noqa
    except ImportError:
        raise ImportError("pandas NOT found. Install pandas for DataFrame "
                          "support.")
    # import cudf here for a lazy import
    try:
        import cudf
    except ImportError:
        raise ImportError("cudf NOT found. Install RAPIDS for CUDA DataFrame "
                          "support.")

    cdf = cudf.concat(
        (
            cudf.from_pandas(
                iteration
                .particles[species_name]
                .to_df(attributes=attributes)
                .assign(iteration=i)
            )
            for i, iteration in series.snapshots().items()
        ),
        axis=0,
        ignore_index=True,
    )

    return cdf
