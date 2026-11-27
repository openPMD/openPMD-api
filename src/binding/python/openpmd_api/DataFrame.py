"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""

import math
import warnings

import numpy as np


def _floating_dtype(floating):
    """Normalize a user-provided floating-point type to a numpy dtype."""
    # note: np.dtype(None) is float64, but None means "keep the stored type"
    if floating is None:
        return None

    dtype = np.dtype(floating)
    if not np.issubdtype(dtype, np.floating):
        raise TypeError(f"floating must be a floating-point type, got {floating!r}.")
    return dtype


def _to_column(data, unit_SI, floating):
    """Scale freshly loaded data to SI units and apply the requested precision."""
    # Scale in the wider of the stored and the requested precision, so that
    # the scaling neither rounds (widening) nor overflows (narrowing).
    if floating is not None and np.issubdtype(data.dtype, np.floating):
        data = data.astype(np.promote_types(data.dtype, floating), copy=False)

    if not math.isclose(1.0, unit_SI):
        data = np.multiply(data, unit_SI)

    # integers become floating-point numbers if they are scaled
    if floating is not None and np.issubdtype(data.dtype, np.floating):
        data = data.astype(floating, copy=False)

    return data


def _check_index(index):
    """Validate the index argument of the dataframe functions."""
    if index is not None and index != "id":
        raise ValueError(f'index must be None or "id", got {index!r}.')


def _require_id(particle_species):
    """Ensure that a particle species can be indexed by its particle id."""
    if "id" not in particle_species or not particle_species["id"].scalar:
        raise ValueError(
            'index="id" requires a scalar "id" record, which is optional in '
            "openPMD. This particle species has the records: "
            f"{sorted(particle_species)}"
        )


def _set_id_index(df, keys):
    """Move the particle id (and optional further columns) to the index."""
    df = df.set_index(keys)

    if not df.index.is_unique:
        warnings.warn(
            "The particle ids in this dataframe are not unique. Operations "
            "that align on the index, e.g., subtracting two dataframes, can "
            "multiply rows.",
            UserWarning,
            stacklevel=3,
        )

    return df


def _iteration_frames(series, species_name, attributes, floating, index):
    """Load a particle species per iteration, with an "iteration" column."""
    for i, iteration in series.snapshots().items():
        particle_species = iteration.particles[species_name]
        if index == "id":
            _require_id(particle_species)  # before reading this iteration
        yield particle_species.to_df(attributes=attributes, floating=floating).assign(
            iteration=i
        )


def particles_to_dataframe(
    particle_species,
    *legacy_args,
    attributes=None,
    slice=None,
    floating=None,
    index=None,
):
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
    floating : numpy.dtype or type or string, optional
        The floating-point type of all floating-point columns, e.g.,
        np.float64. Columns of other types, e.g., the particle id, keep their
        type. Scaling to SI units is performed in the wider of the stored and
        the requested precision. By default, the stored type is kept.
    index : string, optional
        Set to "id" to use the particle id as the index of the dataframe, like
        calling .set_index("id"): "id" is no longer a column. Use this to
        compare particles between dataframes, because operations on two
        dataframes align on their index. By default, the index is the row in
        the loaded data, which is NOT stable between iterations.

    Returns
    -------
    pandas.DataFrame
        A pandas dataframe with particles as rows and openPMD record
        components of the particle_species as columns.

    Raises
    ------
    ImportError
        Raises an exception if pandas is not installed
    TypeError
        Raises an exception if floating is not a floating-point type
    ValueError
        Raises an exception if index is neither None nor "id", or if it is
        "id" and the particle_species has no scalar "id" record

    Warns
    -----
    UserWarning
        If index is "id" and the loaded particle ids are not unique

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

            warnings.warn(
                "The to_df() argument order changed in "
                "openPMD-api 0.17.0!\nThe slice "
                "argument must be passed as a named argument.",
                DeprecationWarning,
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
        raise ImportError("pandas NOT found. Install pandas for DataFrame support.")
    if slice is None:
        slice = np.s_[()]

    # catch invalid arguments before reading any data
    floating = _floating_dtype(floating)
    _check_index(index)
    if index == "id":
        _require_id(particle_species)

    columns = {}

    for record_name, record in particle_species.items():
        for rc_name, rc in record.items():
            if record.scalar:
                column_name = record_name
            else:
                column_name = record_name + "_" + rc_name
            data = rc[slice]
            # data is only filled after this flush
            particle_species.series_flush()
            columns[column_name] = _to_column(data, rc.unit_SI, floating)

    df = pd.DataFrame(columns)

    if attributes is not None:
        for attribute in attributes:
            df[attribute] = particle_species.get_attribute(attribute)
            if floating is not None and pd.api.types.is_float_dtype(df[attribute]):
                df[attribute] = df[attribute].astype(floating)

    if index == "id":
        return _set_id_index(df, "id")

    # set a header for the first column (row index)
    #   note: this is NOT the particle id
    df.index.name = "row"

    return df


def iterations_to_dataframe(
    series, species_name, attributes=None, *, floating=None, index=None
):
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
    floating : numpy.dtype or type or string, optional
        The floating-point type of all floating-point columns, see
        openpmd_api.ParticleSpecies.to_df. By default, the stored type is kept.
    index : string, optional
        Set to "id" to use the iteration and the particle id as a
        pandas.MultiIndex: "iteration" and "id" are no longer columns. Then,
        df.loc[100] selects an iteration with the same index as
        ParticleSpecies.to_df(index="id") and df.xs(42, level="id") selects
        all iterations of a particle. By default, the index is a running
        number that is NOT related to the particle id.

    Returns
    -------
    pandas.DataFrame
        A pandas dataframe with particles as rows and openPMD record
        components of the particle_species as columns. Particles might be
        repeated over multiple iterations and an "iteration" column is
        added, unless index is "id".

    Raises
    ------
    ImportError
        Raises an exception if pandas is not installed
    TypeError
        Raises an exception if floating is not a floating-point type
    ValueError
        Raises an exception if index is neither None nor "id", or if it is
        "id" and the particle species has no scalar "id" record

    Warns
    -----
    UserWarning
        If index is "id" and particle ids are not unique within an iteration

    See Also
    --------
    pandas.DataFrame : the central dataframe object created here
    """
    # import pandas here for a lazy import
    try:
        import pandas as pd
    except ImportError:
        raise ImportError("pandas NOT found. Install pandas for DataFrame support.")

    _check_index(index)

    df = pd.concat(
        _iteration_frames(series, species_name, attributes, floating, index),
        axis=0,
        ignore_index=True,
    )

    if index == "id":
        return _set_id_index(df, ["iteration", "id"])

    return df


def iterations_to_cudf(
    series, species_name, attributes=None, *, floating=None, index=None
):
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
    floating : numpy.dtype or type or string, optional
        The floating-point type of all floating-point columns, see
        openpmd_api.ParticleSpecies.to_df. By default, the stored type is kept.
    index : string, optional
        Set to "id" to use the iteration and the particle id as a
        cudf.MultiIndex: "iteration" and "id" are no longer columns. By
        default, the index is a running number that is NOT related to the
        particle id.

    Returns
    -------
    cudf.DataFrame
        A cuDF (RAPIDS) dataframe with particles as rows and openPMD record
        components of the particle_species as columns. Particles might be
        repeated over multiple iterations and an "iteration" column is
        added, unless index is "id".

    Raises
    ------
    ImportError
        Raises an exception if cuDF (RAPIDS) is not installed
    TypeError
        Raises an exception if floating is not a floating-point type
    ValueError
        Raises an exception if index is neither None nor "id", or if it is
        "id" and the particle species has no scalar "id" record

    Warns
    -----
    UserWarning
        If index is "id" and particle ids are not unique within an iteration

    See Also
    --------
    cudf.DataFrame : the central dataframe object created here
    """
    # import pandas here for a lazy import
    try:
        import pandas  # noqa
    except ImportError:
        raise ImportError("pandas NOT found. Install pandas for DataFrame support.")
    # import cudf here for a lazy import
    try:
        import cudf
    except ImportError:
        raise ImportError("cudf NOT found. Install RAPIDS for CUDA DataFrame support.")

    _check_index(index)

    cdf = cudf.concat(
        (
            cudf.from_pandas(df)
            for df in _iteration_frames(
                series, species_name, attributes, floating, index
            )
        ),
        axis=0,
        ignore_index=True,
    )

    if index == "id":
        return _set_id_index(cdf, ["iteration", "id"])

    return cdf
