"""Module providing mesh like data loading capability.

Author:
    Pawel Ordyna <p.ordyna@hzdr.de>

License:
GPL - 3.0 license. See LICENSE file for details.
"""

import numpy as np
import openpmd_api as pmd
import scipp as sc

from .utils import _unit_dimension_to_scipp


class DataRelay(sc.DataArray):
    """Data relay for loading openPMD meshes into scipp.

    Attributes
    ----------
    series : openpmd_api.Series
        The openPMD series object
    record : openpmd_api.Record
        The openPMD record object associated with the mesh
    record_component : openpmd_api.Record_Component
        The openPMD record component associated with the mesh

    Methods
    -------
    _verify_init():
        Ensures that the data range to load is contiguous.
    __getitem__(*args, **kwargs):
        Retrieves a subset of the data, returning a new DataRelay instance.
    load_data():
        Loads data from the record component based on data array coordinates,
        adjusting for offsets and extents, and updates the DataArray values.

    """

    def _verify_init(self):
        """Verify that the data is contiguous.

        Check if the chosen subset is contiguous in the openPMD storage by checking coordinate
        differences against the expected grid spacing.

        This is needed since openPMD does nto allow us to load strided chunks.
        """
        for dim in self.dims:
            coord = self.coords[dim]
            diffs = coord[dim, 1:] - coord[dim, :-1]
            diffs = diffs.to(unit="m")
            idx = list(self.record.axis_labels).index(dim)
            step = self.record.grid_spacing[idx]
            step *= self.record.grid_unit_SI
            step = step * sc.Unit("m")
            assert sc.allclose(diffs, step), (
                f"The data has to be contiguous! diffs: {diffs}, step: {step}"
            )

    def __init__(self, series, record, record_component, dummy_array, coords):
        """Initialize the DataRelay object.

        :param series: The openPMD series object associated with the data.
        :type series: openpmd_api.Series
        :param record: The openPMD record object associated with the mesh.
        :type record: openpmd_api.Record
        :param record_component: The openPMD record component associated with the mesh.
        :type record_component: openpmd_api.Record_Component
        :param dummy_array: A scipp array used for the dummy interface. It should use as little
            memory as possible. Usually achieved by setting the stride of the values array to 0. Can
            be read- only.
        :type dummy_array: sc.array
        :param coords: A dictionary of coordinates for the DataArray.
        """
        super().__init__(data=dummy_array, coords=coords)
        self.series = series
        self.record = record
        self.record_component = record_component
        self._verify_init()

    def __getitem__(self, *args, **kwargs):
        """Retrieve a subset of the data, returning a new DataRelay instance.

        Override this method from the base class to use the DataRelay initializer and ensure that
        DataRelay is returned and the _verify_init method is used.

        :param args: Forwarded to the base class.
        :type args: tuple
        :param kwargs: Forwarded to the base class.
        :type kwargs: dict
        :return: A new DataRelay instance with the sliced data.
        :rtype: DataRelay
        """
        dummy_data_aray = super().__getitem__(*args, **kwargs)
        return DataRelay(
            series=self.series,
            record=self.record,
            record_component=self.record_component,
            dummy_array=dummy_data_aray.data,
            coords=dummy_data_aray.coords,
        )

    def load_data(self):
        """Load data from the openPMD dataset.

        Loads a chunk based on the current data array coordinates.

        Calculates the offset and extent for each dimension using the data array coordinates. Loads
        the data chunk from the record component, scales it by the unit SI, and returns a new data
        array with loaded values.

        :return: The DataArray instance with the loaded data.
        :rtype: DataRelay
        """
        offset = [0] * self.record_component.ndim
        extent = [0] * self.record_component.ndim
        for dd, dim in enumerate(self.record.axis_labels):
            try:
                start = self.coords[dim].to(unit="m").value
            except sc.DimensionError:
                start = self.coords[dim][0].to(unit="m").value
            start /= self.record.grid_unit_SI
            start -= self.record.grid_global_offset[dd]
            start /= self.record.grid_spacing[dd]
            start -= self.record_component.position[dd]
            offset[dd] = int(round(start))
            extent[dd] = self.coords[dim].size
        data = self.record_component.load_chunk(offset=offset, extent=extent)
        self.series.flush()
        data *= self.record_component.unit_SI
        data = np.squeeze(data)
        data_array = self.copy()
        data_array.values = data
        return data_array


def get_field_data_relay(series, iteration, field, component=pmd.Mesh_Record_Component.SCALAR):
    """Get openPMD mesh as a data relay.

    Create a DataRelay object for a specified field and component in an openPMD series.

    :param series: The openPMD series containing the data.
    :type series: openpmd_api.Series
    :param iteration: The iteration number to access within the series.
    :type iteration: int
    :param field: The name of the field to retrieve.
    :type field: str
    :param component: The component of the field to retrieve, default is SCALAR.
    :type component: openpmd_api.Mesh_Record_Component, optional
    :return: A DataRelay instance initialized with the specified field and component data.
    :rtype: DataRelay
    """
    record = series.iterations[iteration].meshes[field]
    rc = record[component]
    dims = record.axis_labels
    time = (series.iterations[iteration].time + record.time_offset) * series.iterations[
        iteration
    ].time_unit_SI
    time = sc.scalar(time, unit="s", dtype="double")
    coords = {"t": time}
    for dd, dim in enumerate(dims):
        length = rc.shape[dd]
        start = record.grid_global_offset[dd]
        step = record.grid_spacing[dd]
        values = np.arange(length, dtype=np.float64)
        values *= step
        values += rc.position[dd] * step
        values += start
        values *= record.grid_unit_SI
        coord = sc.array(dims=[dim], values=values, unit="m")
        coords[dim] = coord

    small = np.zeros(1, dtype=rc.dtype)
    dummy_array = np.lib.stride_tricks.as_strided(
        small, shape=rc.shape, strides=[0] * rc.ndim, writeable=False
    )
    dummy_array = sc.array(
        dims=dims, values=dummy_array, unit=_unit_dimension_to_scipp(record.unit_dimension)
    )

    return DataRelay(
        series=series, record=record, record_component=rc, dummy_array=dummy_array, coords=coords
    )


def get_field(series, iteration, field, component=pmd.Mesh_Record_Component.SCALAR):
    """Retrieve and load openPMD mesh data without slicing.

    This function creates a DataRelay object for a specified field and component in an openPMD
    series, loads the whole mesh, and returns the resulting DataArray.

    :param series: The openPMD series containing the data.
    :type series: openpmd_api.Series
    :param iteration: The iteration number to access within the series.
    :type iteration: int
    :param field: The name of the field to retrieve.
    :type field: str
    :param component: The component of the field to retrieve, default is SCALAR.
    :type component: openpmd_api.Mesh_Record_Component, optional
    :return: A DataArray instance with the loaded data.
    :rtype: DataRelay
    """
    return get_field_data_relay(series, iteration, field, component).load_data()
