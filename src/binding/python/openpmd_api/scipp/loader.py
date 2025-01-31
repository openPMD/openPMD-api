"""Module providing the DataLoader class.

Provides the main openPMD to scpp interface class.

Author:
    Pawel Ordyna <p.ordyna@hzdr.de>

License:
GPL - 3.0 license. See LICENSE file for details.
"""

import openpmd_api as pmd
import scipp as sc

from .mesh_loader import get_field, get_field_data_relay
from .utils import closest


def get_time_axis(series):
    """Get the time axis from an openPMD series.

    :param series: The openPMD series containing the data.
    :type series: openpmd_api.Series
    :return: A Scipp array representing the time axis.
    :rtype: sc.DataArray
    """
    t = [
        series.iterations[it].time * series.iterations[it].time_unit_SI for it in series.iterations
    ]
    return sc.array(dims=["t"], values=t, unit="s", dtype="double")


def get_iterations(series):
    """Get the iterations from an openPMD series.

    :param series: The openPMD series containing the data.
    :type series: openpmd_api.Series
    :return: A Scipp dataset containing iteration IDs and their corresponding times.
    :rtype: sc.Dataset
    """
    t = get_time_axis(series)
    return sc.Dataset(
        data={
            "iteration_id": sc.DataArray(
                data=sc.array(dims=["t"], values=list(series.iterations)), coords={"t": t}
            )
        }
    )


class DataLoader:
    """DataLoader class for loading and retrieving openPMD mesh data.

    This class initializes an openPMD series from a given file path and provides
    methods to retrieve mesh data fields either by iteration index or by time.
    The data can be retrieved as a DataRelay object or directly as a DataArray.

    Attributes:
        series (openpmd_api.Series): The openPMD series initialized from the file path.
        iterations (sc.Dataset): A dataset containing iteration IDs and their corresponding times.

    """

    def __init__(self, path):
        """Initialize the DataLoader with an openPMD series from the specified file path.

        :param path: The file path to the openPMD data file.
        :type path: str

        Initializes the `series` attribute as an openPMD series in read-only mode
        and the `iterations` attribute as a Scipp dataset containing iteration IDs
        and their corresponding times.
        """
        self.series = pmd.Series(str(path), pmd.Access.read_only)
        self.iterations = get_iterations(self.series)

    def get_field(
        self,
        field,
        component=pmd.Mesh_Record_Component.SCALAR,
        time=None,
        iteration=None,
        relay=False,
        time_tolerance=10 * sc.Unit("fs"),
    ):
        """Retrieve a mesh data field from the openPMD series.

        This method retrieves a specified field and component from the
        openPMD series either by iteration index or by time. The data
        can be returned as a DataRelay object or directly as a
        DataArray.

        :param field: The name of the field to retrieve.
        :type field: str
        :param component: The component of the field to retrieve,
            default is SCALAR.
        :type component: openpmd_api.Mesh_Record_Component, optional
        :param time: The time at which to retrieve the field. Either
            time or iteration must be provided, but not both.
        :type time: sc.Variable, optional
        :param iteration: The iteration index at which to retrieve the
            field. Either time or iteration must be provided, but not
            both.
        :type iteration: int, optional
        :param relay: If True, return the data as a DataRelay object;
            otherwise, return as a DataArray.
        :type relay: bool, optional
        :param time_tolerance: The tolerance for matching the time when
            retrieving by time, default is 10 femtoseconds.
        :type time_tolerance: sc.Unit, optional
        :return: The requested field data as a DataRelay or DataArray.
        :rtype: DataRelay or DataArray
        :raises AssertionError: If neither time nor iteration is
            provided, or if both are provided.
        :raises IndexError: If no iteration is found within the
            specified time tolerance.
        """
        assert (time is None and iteration is not None) or (
            iteration is None and time is not None
        ), "Provide either iteration index or time"
        if iteration is None:
            # handle integer  inputs
            time = time.astype("double")
            time_tolerance = time_tolerance.astype("double")
            time = time.to(unit=self.iterations["iteration_id"].coords["t"].unit)
            try:
                iteration = int(self.iterations["iteration_id"]["t", time].value)
            except IndexError:
                idx = closest(self.iterations["iteration_id"], "t", time)
                iteration = self.iterations["iteration_id"]["t", idx]
                assert time_tolerance is None or sc.abs(
                    iteration.coords["t"] - time
                ) <= time_tolerance.to(unit=time.unit), (
                    f"No iteration found within time_tolerance={time_tolerance}."
                )
                print(
                    "Series does not contain iteration at the exact time. "
                    "Using closest iteration instead.",
                    flush=True,
                )
                iteration = int(iteration.value)

        if relay:
            return get_field_data_relay(
                series=self.series, field=field, component=component, iteration=iteration
            )
        else:
            return get_field(
                series=self.series, field=field, component=component, iteration=iteration
            )
