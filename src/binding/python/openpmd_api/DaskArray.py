"""
This file is part of the openPMD-api.

Copyright 2021 openPMD contributors
Authors: Axel Huebl
License: LGPLv3+
"""
import math

import numpy as np

try:
    from dask.array import from_array
    found_dask = True
except ImportError:
    found_dask = False


class DaskRecordComponent:
    # shape, .ndim, .dtype and support numpy-style slicing
    def __init__(self, record_component):
        self.rc = record_component

    @property
    def shape(self):
        # fixme: https://github.com/openPMD/openPMD-api/issues/808
        return tuple(self.rc.shape)

    @property
    def ndim(self):
        return self.rc.ndim

    @property
    def dtype(self):
        return self.rc.dtype

    def __getitem__(self, slices):
        """here we support what Record_Component implements: a tuple of slices,
        a slice or an index; we do not support fancy indexing
        """
        # FIXME: implement handling of zero-slices in Record_Component
        # https://github.com/openPMD/openPMD-api/issues/957
        all_zero = True
        for s in slices:
            if s != np.s_[0:0]:
                all_zero = False
        if all_zero:
            return np.array([], dtype=self.dtype)

        data = self.rc[slices]
        self.rc.series_flush()
        if not math.isclose(1.0, self.rc.unit_SI):
            data = np.multiply(data, self.rc.unit_SI)

        return data


def record_component_to_daskarray(record_component):
    """
    Load a RecordComponent into a Dask.array.

    Parameters
    ----------
    record_component : openpmd_api.Record_Component
        A record component class in openPMD-api.

    Returns
    -------
    dask.array
        A dask array.

    Raises
    ------
    ImportError
        Raises an exception if dask is not installed

    See Also
    --------
    openpmd_api.BaseRecordComponent.available_chunks : available chunks that
        are used internally to parallelize reading
    dask.array : the (potentially distributed) array object created here
    """
    if not found_dask:
        raise ImportError("dask NOT found. Install dask for Dask DataFrame "
                          "support.")

    # get optimal chunks
    chunks = record_component.available_chunks()

    # sort and prepare the chunks for Dask's array API
    #   https://docs.dask.org/en/latest/array-chunks.html
    #   https://docs.dask.org/en/latest/array-api.html?highlight=from_array#other-functions
    # sorted and unique
    offsets_per_dim = list(map(list, zip(*[chunk.offset for chunk in chunks])))
    offsets_sorted_unique_per_dim = [sorted(set(o)) for o in offsets_per_dim]

    # print("offsets_sorted_unique_per_dim=",
    #       list(offsets_sorted_unique_per_dim))

    # case 1: PIConGPU static load balancing (works with Dask assumptions,
    #                                         chunk option no. 3)
    #   all chunks in the same column have the same column width although
    #   individual columns have different widths
    # case 2: AMReX boxes
    #   all chunks are multiple of a common block size, offsets are a multiple
    #   of a common blocksize
    #   problem: too limited description in Dask
    #     https://github.com/dask/dask/issues/7475
    #   work-around: create smaller chunks (this incurs a read cost) by forcing
    #                into case 1
    #                (this can lead to larger blocks than using the gcd of the
    #                 extents aka AMReX block size)
    common_chunk_widths_per_dim = list()
    for d, offsets_in_dim in enumerate(offsets_sorted_unique_per_dim):
        # print("d=", d, offsets_in_dim, record_component.shape[d])
        offsets_in_dim_arr = np.array(offsets_in_dim)
        # note: this is in the right order of rows/columns, contrary to a
        #       sorted extent list from chunks
        extents_in_dim = np.zeros_like(offsets_in_dim_arr)
        extents_in_dim[:-1] = offsets_in_dim_arr[1:]
        extents_in_dim[-1] = record_component.shape[d]
        if len(extents_in_dim) > 1:
            extents_in_dim[:-1] -= offsets_in_dim_arr[:-1]
            extents_in_dim[-1] -= offsets_in_dim_arr[-1]
        # print("extents_in_dim=", extents_in_dim)
        common_chunk_widths_per_dim.append(tuple(extents_in_dim))

    common_chunk_widths_per_dim = tuple(common_chunk_widths_per_dim)
    # print("common_chunk_widths_per_dim=", common_chunk_widths_per_dim)

    da = from_array(
        DaskRecordComponent(record_component),
        chunks=common_chunk_widths_per_dim,
        # name=None,
        asarray=True,
        fancy=False,
        # getitem=None,
        # meta=None,
        # inline_array=False
    )

    return da
