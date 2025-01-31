"""Utility module.

Author:
    Pawel Ordyna <p.ordyna@hzdr.de>

License:
GPL - 3.0 license. See LICENSE file for details.
"""

import numpy as np
import scipp as sc


def _unit_dimension_to_scipp(unit_dimension):
    """Convert a unit dimension from the openPMD standard to a Scipp unit.

    This function takes a tuple representing the powers of the seven base SI units
    (length, mass, time, electric current, thermodynamic temperature, amount of substance,
    and luminous intensity) and converts it into a Scipp unit. The conversion is based on
    the openPMD standard, which describes units as powers of these base measures.

    :param tuple unit_dimension: A tuple containing seven integers, each representing
    the power of a base SI unit in the order: (length, mass, time, electric current,
    thermodynamic temperature, amount of substance, luminous intensity).
     For example, (1, 0, -2, 0, 0, 0, 0) corresponds to meters per second squared (m/s²).

    :returns: A Scipp unit object representing the combined unit as specified by the input
        unit dimensions.
    :rtype: sc.Unit

    :example:
        >>> _unit_dimension_to_scipp((1, 0, -2, 0, 0, 0, 0))
        Unit('m/s^2')

    :notes:
        - The function assumes that the input tuple has exactly seven elements.
        - Each element in the tuple corresponds to the power of a specific base unit.
    """
    # unit dimension description from the openPMD standard:
    # powers of the 7 base measures characterizing the record's unit in SI
    # (length L, mass M, time T, electric current I, thermodynamic temperature theta,
    # amount of substance N, luminous intensity J)
    base_units = (
        1.0 * sc.Unit("m"),
        1.0 * sc.Unit("kg"),
        1.0 * sc.Unit("s"),
        1.0 * sc.Unit("A"),
        1.0 * sc.Unit("K"),
        1.0 * sc.Unit("mol"),
        1.0 * sc.Unit("cd"),
    )
    unit = 1.0 * sc.Unit("1")
    for dim, base_unit in zip(unit_dimension, base_units, strict=False):
        if dim != 0:
            unit *= base_unit**dim
    return unit.unit


def closest(data, dim, val):
    """Find the index of the closest value in a dataset along a specified dimension.

    This function calculates the index of the element in the specified dimension
    of the dataset that is closest to the given value. It ensures that the value
    is converted to the same unit as the dimension's coordinate before performing
    the comparison.

    :param data: The data array containing the dimension to search.
    :type data: sc.DataArray
    :param dim: The name of the dimension along which to find the closest value.
    :type dim: str
    :param val: The value to compare against, which will be converted to the unit of
                the dimension's coordinate.
    :type val: sc.Variable

    :return: The index of the closest value in the specified dimension.
    :rtype: int

    :notes:
        - The function assumes that `val` can be converted to the unit of the
          dimension's coordinate.
        - The dataset `data` must have coordinates defined for the specified dimension.
    """
    val = val.astype("double")
    coord = data.coords[dim]
    val = val.to(unit=coord.unit)
    return np.argmin(sc.abs(coord - val).values)
