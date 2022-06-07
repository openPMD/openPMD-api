/* Copyright 2018-2021 Axel Huebl
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "openPMD/DatatypeHelpers.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/ShareRaw.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/binding/python/Numpy.hpp"
#include "openPMD/binding/python/Pickle.hpp"

#include <algorithm>
#include <complex>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

namespace py = pybind11;
using namespace openPMD;

/** Convert a py::tuple of py::slices to Offset & Extent
 *
 * https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.indexing.html
 * https://github.com/numpy/numpy/blob/v1.16.1/numpy/core/src/multiarray/mapping.c#L348-L375
 */
inline std::tuple<Offset, Extent, std::vector<bool>> parseTupleSlices(
    uint8_t const ndim, Extent const &full_extent, py::tuple const &slices)
{
    uint8_t const numSlices = py::len(slices);

    Offset offset(ndim, 0u);
    Extent extent(ndim, 1u);
    std::vector<bool> flatten(ndim, false);
    int16_t curAxis = -1;

    int16_t posEllipsis = -1;
    for (uint8_t i = 0u; i < numSlices; ++i)
    {
        ++curAxis;

        if (i >= ndim && posEllipsis == -1 && slices[i].ptr() != Py_Ellipsis)
            throw py::index_error(
                "too many indices for dimension of record component!");

        if (slices[i].ptr() == Py_Ellipsis)
        {
            // only allowed once
            if (posEllipsis != -1)
                throw py::index_error(
                    "an index can only have a single ellipsis ('...')");
            posEllipsis = curAxis;

            // might be omitted if all other indices are given as well
            if (numSlices == ndim + 1)
            {
                --curAxis;
                continue;
            }

            // how many slices were given after the ellipsis
            uint8_t const numSlicesAfterEllipsis =
                numSlices - uint8_t(posEllipsis) - 1u;
            // how many slices does the ellipsis represent
            uint8_t const numSlicesEllipsis = numSlices -
                uint8_t(posEllipsis) // slices before
                - numSlicesAfterEllipsis; // slices after

            // fill ellipsis indices
            // note: if enough further indices are given, the ellipsis
            //       might stand for no axis: valid and ignored
            for (; curAxis < posEllipsis + int16_t(numSlicesEllipsis);
                 ++curAxis)
            {
                offset.at(curAxis) = 0;
                extent.at(curAxis) = full_extent.at(curAxis);
            }
            --curAxis;

            continue;
        }

        if (PySlice_Check(slices[i].ptr()))
        {
            py::slice slice = py::cast<py::slice>(slices[i]);

            size_t start, stop, step, slicelength;
            if (!slice.compute(
                    full_extent.at(curAxis),
                    &start,
                    &stop,
                    &step,
                    &slicelength))
                throw py::error_already_set();

            // TODO PySlice_AdjustIndices: Python 3.6.1+
            //      Adjust start/end slice indices assuming a sequence of the
            //      specified length. Out of bounds indices are clipped in a
            //      manner consistent with the handling of normal slices.
            // slicelength = PySlice_AdjustIndices(full_extent[curAxis],
            // (ssize_t*)&start, (ssize_t*)&stop, step);

            if (step != 1u)
                throw py::index_error(
                    "strides in selection are inefficient, not implemented!");

            // verified for size later in C++ API
            offset.at(curAxis) = start;
            extent.at(curAxis) = slicelength; // stop - start;

            continue;
        }

        try
        {
            auto const index = py::cast<std::int64_t>(slices[i]);

            if (index < 0)
                offset.at(curAxis) = full_extent.at(curAxis) + index;
            else
                offset.at(curAxis) = index;

            extent.at(curAxis) = 1;
            flatten.at(curAxis) = true; // indices flatten the dimension

            if (offset.at(curAxis) >= full_extent.at(curAxis))
                throw py::index_error(
                    std::string("index ") + std::to_string(offset.at(curAxis)) +
                    std::string(" is out of bounds for axis ") +
                    std::to_string(i) + std::string(" with size ") +
                    std::to_string(full_extent.at(curAxis)));

            continue;
        }
        catch (const py::cast_error &e)
        {
            // not an index
        }

        if (slices[i].ptr() == Py_None)
        {
            // py::none newaxis = py::cast< py::none >( slices[i] );;
            throw py::index_error("None (newaxis) not implemented!");

            // continue;
        }

        // if we get here, the last slice type was not successfully processed
        --curAxis;
        throw py::index_error(
            std::string("unknown index type passed: ") +
            py::str(slices[i]).cast<std::string>());
    }

    // fill omitted higher indices with "select all"
    for (++curAxis; curAxis < int16_t(ndim); ++curAxis)
    {
        extent.at(curAxis) = full_extent.at(curAxis);
    }

    return std::make_tuple(offset, extent, flatten);
}

/** Check an array is a contiguous buffer
 *
 * Required are contiguous buffers for store and load
 *
 * - not strided with paddings
 * - not a view in another buffer that results in striding
 */
inline void check_buffer_is_contiguous(py::array &a)
{

    auto *view = new Py_buffer();
    int flags = PyBUF_STRIDES | PyBUF_FORMAT;
    if (PyObject_GetBuffer(a.ptr(), view, flags) != 0)
    {
        delete view;
        throw py::error_already_set();
    }
    bool isContiguous = (PyBuffer_IsContiguous(view, 'A') != 0);
    PyBuffer_Release(view);
    delete view;

    if (!isContiguous)
        throw py::index_error(
            "strides in chunk are inefficient, not implemented!");
    // @todo in order to implement stride handling, one needs to
    //       loop over the input data strides in store/load calls
}

/** Store Chunk
 *
 * Called with offset and extent that are already in the record component's
 * dimension.
 *
 * Size checks of the requested chunk (spanned data is in valid bounds)
 * will be performed at C++ API part in RecordComponent::storeChunk .
 */
inline void store_chunk(
    RecordComponent &r,
    py::array &a,
    Offset const &offset,
    Extent const &extent,
    std::vector<bool> const &flatten)
{
    // @todo keep locked until flush() is performed
    // a.flags.writable = false;
    // a.flags.owndata = false;

    // verify offset + extend fit in dataset extent

    //   some one-size dimensions might be flattended in our r due to selections
    //   by index
    size_t const numFlattenDims =
        std::count(flatten.begin(), flatten.end(), true);
    auto const r_extent = r.getExtent();
    auto const s_extent(extent); // selected extent in r
    std::vector<std::uint64_t> r_shape(r_extent.size() - numFlattenDims);
    std::vector<std::uint64_t> s_shape(s_extent.size() - numFlattenDims);
    auto maskIt = flatten.begin();
    std::copy_if(
        std::begin(r_extent),
        std::end(r_extent),
        std::begin(r_shape),
        [&maskIt](std::uint64_t) { return !*(maskIt++); });
    maskIt = flatten.begin();
    std::copy_if(
        std::begin(s_extent),
        std::end(s_extent),
        std::begin(s_shape),
        [&maskIt](std::uint64_t) { return !*(maskIt++); });

    //   verify shape and extent
    if (size_t(a.ndim()) != r_shape.size())
        throw py::index_error(
            std::string("dimension of chunk (") + std::to_string(a.ndim()) +
            std::string("D) does not fit dimension of selection "
                        "in record component (") +
            std::to_string(r_shape.size()) + std::string("D)"));

    for (auto d = 0; d < a.ndim(); ++d)
    {
        // selection causes overflow of r
        if (offset.at(d) + extent.at(d) > r_shape.at(d))
            throw py::index_error(
                std::string("slice ") + std::to_string(offset.at(d)) +
                std::string(":") + std::to_string(extent.at(d)) +
                std::string(" is out of bounds for axis ") + std::to_string(d) +
                std::string(" with size ") + std::to_string(r_shape.at(d)));
        // underflow of selection in r for given a
        if (s_shape.at(d) != std::uint64_t(a.shape()[d]))
            throw py::index_error(
                std::string("size of chunk (") + std::to_string(a.shape()[d]) +
                std::string(") for axis ") + std::to_string(d) +
                std::string(" does not match selection ") +
                std::string("size in record component (") +
                std::to_string(s_extent.at(d)) + std::string(")"));
    }

    check_buffer_is_contiguous(a);

    // here, we increase a reference on the user-passed data so that
    // temporary and lost-scope variables stay alive until we flush
    // note: this does not yet prevent the user, as in C++, to build
    //       a race condition by manipulating the data they passed
    auto store_data = [&r, &a, &offset, &extent](auto cxxtype) {
        using CXXType = decltype(cxxtype);
        a.inc_ref();
        void *data = a.mutable_data();
        std::shared_ptr<CXXType> shared(
            (CXXType *)data, [a](CXXType *) { a.dec_ref(); });
        r.storeChunk(std::move(shared), offset, extent);
    };

    // store
    auto const dtype = dtype_from_numpy(a.dtype());
    if (dtype == Datatype::CHAR)
        store_data(char());
    else if (dtype == Datatype::UCHAR)
        store_data((unsigned char)0);
    else if (dtype == Datatype::SHORT)
        store_data(short());
    else if (dtype == Datatype::INT)
        store_data(int());
    else if (dtype == Datatype::LONG)
        store_data(long());
    else if (dtype == Datatype::LONGLONG)
        store_data((long long)0);
    else if (dtype == Datatype::USHORT)
        store_data((unsigned short)0);
    else if (dtype == Datatype::UINT)
        store_data((unsigned int)0);
    else if (dtype == Datatype::ULONG)
        store_data((unsigned long)0);
    else if (dtype == Datatype::ULONGLONG)
        store_data((unsigned long long)0);
    else if (dtype == Datatype::LONG_DOUBLE)
        store_data((long double)0);
    else if (dtype == Datatype::DOUBLE)
        store_data(double());
    else if (dtype == Datatype::FLOAT)
        store_data(float());
    else if (dtype == Datatype::CLONG_DOUBLE)
        store_data(std::complex<long double>());
    else if (dtype == Datatype::CDOUBLE)
        store_data(std::complex<double>());
    else if (dtype == Datatype::CFLOAT)
        store_data(std::complex<float>());
    /* @todo
    .value("STRING", Datatype::STRING)
    .value("VEC_STRING", Datatype::VEC_STRING)
    .value("ARR_DBL_7", Datatype::ARR_DBL_7)
    */
    else if (dtype == Datatype::BOOL)
        store_data(bool());
    else
        throw std::runtime_error(
            std::string("Datatype '") + std::string(py::str(a.dtype())) +
            std::string("' not known in 'storeChunk'!"));
}

/** Store Chunk
 *
 * Called with a py::tuple of slices and a py::array
 */
inline void
store_chunk(RecordComponent &r, py::array &a, py::tuple const &slices)
{
    uint8_t ndim = r.getDimensionality();
    auto const full_extent = r.getExtent();

    Offset offset;
    Extent extent;
    std::vector<bool> flatten;
    std::tie(offset, extent, flatten) =
        parseTupleSlices(ndim, full_extent, slices);

    store_chunk(r, a, offset, extent, flatten);
}

struct PythonDynamicMemoryView
{
    using ShapeContainer = pybind11::array::ShapeContainer;

    template <typename T>
    PythonDynamicMemoryView(
        DynamicMemoryView<T> dynamicView,
        ShapeContainer arrayShape,
        ShapeContainer strides)
        : m_dynamicView(std::shared_ptr<void>(
              new DynamicMemoryView<T>(std::move(dynamicView))))
        , m_arrayShape(std::move(arrayShape))
        , m_strides(std::move(strides))
        , m_datatype(determineDatatype<T>())
    {}

    pybind11::memoryview currentView() const;

    std::shared_ptr<void> m_dynamicView;
    ShapeContainer m_arrayShape;
    ShapeContainer m_strides;
    Datatype m_datatype;
};

namespace
{
struct GetCurrentView
{
    template <typename T>
    pybind11::memoryview operator()(PythonDynamicMemoryView const &dynamicView)
    {
        auto span =
            static_cast<DynamicMemoryView<T> *>(dynamicView.m_dynamicView.get())
                ->currentBuffer();
        return py::memoryview::from_buffer(
            span.data(),
            dynamicView.m_arrayShape,
            dynamicView.m_strides,
            /* readonly = */ false);
    }

    std::string errorMsg = "DynamicMemoryView";
};

template <>
pybind11::memoryview
GetCurrentView::operator()<std::string>(PythonDynamicMemoryView const &)
{
    throw std::runtime_error("[DynamicMemoryView] Only PODs allowed.");
}
} // namespace

pybind11::memoryview PythonDynamicMemoryView::currentView() const
{
    static GetCurrentView const cv;
    return switchNonVectorType(m_datatype, cv, *this);
}

namespace
{
struct StoreChunkSpan
{
    template <typename T>
    PythonDynamicMemoryView
    operator()(RecordComponent &r, Offset const &offset, Extent const &extent)
    {
        DynamicMemoryView<T> dynamicView = r.storeChunk<T>(offset, extent);
        pybind11::array::ShapeContainer arrayShape(
            extent.begin(), extent.end());
        std::vector<py::ssize_t> strides(extent.size());
        {
            py::ssize_t accumulator = sizeof(T);
            size_t dim = extent.size();
            while (dim > 0)
            {
                --dim;
                strides[dim] = accumulator;
                accumulator *= extent[dim];
            }
        }
        return PythonDynamicMemoryView(
            std::move(dynamicView),
            std::move(arrayShape),
            py::array::ShapeContainer(std::move(strides)));
    }

    std::string errorMsg = "RecordComponent.store_chunk()";
};

template <>
PythonDynamicMemoryView StoreChunkSpan::operator()<std::string>(
    RecordComponent &, Offset const &, Extent const &)
{
    throw std::runtime_error(
        "[RecordComponent.store_chunk()] Only PODs allowed.");
}
} // namespace

inline PythonDynamicMemoryView store_chunk_span(
    RecordComponent &r,
    Offset const &offset,
    Extent const &extent,
    std::vector<bool> const &flatten)
{
    // some one-size dimensions might be flattended in our output due to
    // selections by index
    size_t const numFlattenDims =
        std::count(flatten.begin(), flatten.end(), true);
    std::vector<ptrdiff_t> shape(extent.size() - numFlattenDims);
    auto maskIt = flatten.begin();
    std::copy_if(
        std::begin(extent),
        std::end(extent),
        std::begin(shape),
        [&maskIt](std::uint64_t) { return !*(maskIt++); });

    static StoreChunkSpan scs;
    return switchNonVectorType(r.getDatatype(), scs, r, offset, extent);
}

inline PythonDynamicMemoryView
store_chunk_span(RecordComponent &r, py::tuple const &slices)
{
    uint8_t ndim = r.getDimensionality();
    auto const full_extent = r.getExtent();

    Offset offset;
    Extent extent;
    std::vector<bool> flatten;
    std::tie(offset, extent, flatten) =
        parseTupleSlices(ndim, full_extent, slices);

    return store_chunk_span(r, offset, extent, flatten);
}

/** Load Chunk
 *
 * Called with offset and extent that are already in the record component's
 * dimension.
 *
 * Size checks of the requested chunk (spanned data is in valid bounds)
 * will be performed at C++ API part in RecordComponent::loadChunk .
 */
void load_chunk(
    RecordComponent &r,
    py::buffer &buffer,
    Offset const &offset,
    Extent const &extent)
{
    auto const dtype = dtype_to_numpy(r.getDatatype());
    py::buffer_info buffer_info = buffer.request(/* writable = */ true);

    auto const &strides = buffer_info.strides;
    // this function requires a contiguous slab of memory, so check the strides
    // whether we have that
    if (strides.size() == 0)
    {
        throw std::runtime_error(
            "[Record_Component::load_chunk()] Empty buffer passed.");
    }
    {
        py::ssize_t accumulator = toBytes(r.getDatatype());
        size_t dim = strides.size();
        while (dim > 0)
        {
            --dim;
            if (strides[dim] != accumulator)
            {
                throw std::runtime_error(
                    "[Record_Component::load_chunk()] Requires contiguous slab"
                    " of memory.");
            }
            accumulator *= extent[dim];
        }
    }

    // here, we increase a reference on the user-passed data so that
    // temporary and lost-scope variables stay alive until we flush
    // note: this does not yet prevent the user, as in C++, to build
    //       a race condition by manipulating the data they passed
    auto load_data =
        [&r, &buffer, &buffer_info, &offset, &extent](auto cxxtype) {
            using CXXType = decltype(cxxtype);
            buffer.inc_ref();
            // buffer_info.inc_ref();
            void *data = buffer_info.ptr;
            std::shared_ptr<CXXType> shared(
                (CXXType *)data, [buffer](CXXType *) { buffer.dec_ref(); });
            r.loadChunk(std::move(shared), offset, extent);
        };

    if (r.getDatatype() == Datatype::CHAR)
        load_data((char)0);
    else if (r.getDatatype() == Datatype::UCHAR)
        load_data((unsigned char)0);
    else if (r.getDatatype() == Datatype::SHORT)
        load_data((short)0);
    else if (r.getDatatype() == Datatype::INT)
        load_data((int)0);
    else if (r.getDatatype() == Datatype::LONG)
        load_data((long)0);
    else if (r.getDatatype() == Datatype::LONGLONG)
        load_data((long long)0);
    else if (r.getDatatype() == Datatype::USHORT)
        load_data((unsigned short)0);
    else if (r.getDatatype() == Datatype::UINT)
        load_data((unsigned int)0);
    else if (r.getDatatype() == Datatype::ULONG)
        load_data((unsigned long)0);
    else if (r.getDatatype() == Datatype::ULONGLONG)
        load_data((unsigned long long)0);
    else if (r.getDatatype() == Datatype::LONG_DOUBLE)
        load_data((long double)0);
    else if (r.getDatatype() == Datatype::DOUBLE)
        load_data((double)0);
    else if (r.getDatatype() == Datatype::FLOAT)
        load_data((float)0);
    else if (r.getDatatype() == Datatype::CLONG_DOUBLE)
        load_data((std::complex<long double>)0);
    else if (r.getDatatype() == Datatype::CDOUBLE)
        load_data((std::complex<double>)0);
    else if (r.getDatatype() == Datatype::CFLOAT)
        load_data((std::complex<float>)0);
    else if (r.getDatatype() == Datatype::BOOL)
        load_data((bool)0);
    else
        throw std::runtime_error(
            std::string("Datatype not known in 'loadChunk'!"));
}

/** Load Chunk
 *
 * Called with offset and extent that are already in the record component's
 * dimension.
 *
 * Size checks of the requested chunk (spanned data is in valid bounds)
 * will be performed at C++ API part in RecordComponent::loadChunk .
 */
inline void load_chunk(
    RecordComponent &r,
    py::array &a,
    Offset const &offset,
    Extent const &extent)
{
    // check array is large enough
    size_t s_load = 1u;
    size_t s_array = 1u;
    std::string str_extent_shape;
    std::string str_array_shape;
    for (auto &si : extent)
    {
        s_load *= si;
        str_extent_shape.append(" ").append(std::to_string(si));
    }
    for (py::ssize_t d = 0; d < a.ndim(); ++d)
    {
        s_array *= a.shape()[d];
        str_array_shape.append(" ").append(std::to_string(a.shape()[d]));
    }

    /* we allow flattening of the result dimension
    if( size_t(a.ndim()) > extent.size() )
        throw py::index_error(
            std::string("dimension of array (") +
            std::to_string(a.ndim()) +
            std::string("D) does not fit dimension of selection "
                        "in record component (") +
            std::to_string(extent.size()) +
            std::string("D)")
        );
    */
    if (s_array < s_load)
    {
        throw py::index_error(
            std::string("size of array (") + std::to_string(s_array) +
            std::string("; shape:") + str_array_shape +
            std::string(") is smaller than size of selection "
                        "in record component (") +
            std::to_string(s_load) + std::string("; shape:") +
            str_extent_shape + std::string(")"));
    }

    check_buffer_is_contiguous(a);

    // here, we increase a reference on the user-passed data so that
    // temporary and lost-scope variables stay alive until we flush
    // note: this does not yet prevent the user, as in C++, to build
    //       a race condition by manipulating the data they passed
    auto load_data = [&r, &a, &offset, &extent](auto cxxtype) {
        using CXXType = decltype(cxxtype);
        a.inc_ref();
        void *data = a.mutable_data();
        std::shared_ptr<CXXType> shared(
            (CXXType *)data, [a](CXXType *) { a.dec_ref(); });
        r.loadChunk(std::move(shared), offset, extent);
    };

    if (r.getDatatype() == Datatype::CHAR)
        load_data(char());
    else if (r.getDatatype() == Datatype::UCHAR)
        load_data((unsigned char)0);
    else if (r.getDatatype() == Datatype::SHORT)
        load_data(short());
    else if (r.getDatatype() == Datatype::INT)
        load_data(int());
    else if (r.getDatatype() == Datatype::LONG)
        load_data(long());
    else if (r.getDatatype() == Datatype::LONGLONG)
        load_data((long long)0);
    else if (r.getDatatype() == Datatype::USHORT)
        load_data((unsigned short)0);
    else if (r.getDatatype() == Datatype::UINT)
        load_data((unsigned int)0);
    else if (r.getDatatype() == Datatype::ULONG)
        load_data((unsigned long)0);
    else if (r.getDatatype() == Datatype::ULONGLONG)
        load_data((unsigned long long)0);
    else if (r.getDatatype() == Datatype::LONG_DOUBLE)
        load_data((long double)0);
    else if (r.getDatatype() == Datatype::DOUBLE)
        load_data(double());
    else if (r.getDatatype() == Datatype::FLOAT)
        load_data(float());
    else if (r.getDatatype() == Datatype::CLONG_DOUBLE)
        load_data(std::complex<long double>());
    else if (r.getDatatype() == Datatype::CDOUBLE)
        load_data(std::complex<double>());
    else if (r.getDatatype() == Datatype::CFLOAT)
        load_data(std::complex<float>());
    else if (r.getDatatype() == Datatype::BOOL)
        load_data(bool());
    else
        throw std::runtime_error(
            std::string("Datatype not known in 'load_chunk'!"));
}

/** Load Chunk
 *
 * Called with a py::tuple of slices.
 */
inline py::array load_chunk(RecordComponent &r, py::tuple const &slices)
{
    uint8_t ndim = r.getDimensionality();
    auto const full_extent = r.getExtent();

    Offset offset;
    Extent extent;
    std::vector<bool> flatten;
    std::tie(offset, extent, flatten) =
        parseTupleSlices(ndim, full_extent, slices);

    // some one-size dimensions might be flattended in our output due to
    // selections by index
    size_t const numFlattenDims =
        std::count(flatten.begin(), flatten.end(), true);
    std::vector<ptrdiff_t> shape(extent.size() - numFlattenDims);
    auto maskIt = flatten.begin();
    std::copy_if(
        std::begin(extent),
        std::end(extent),
        std::begin(shape),
        [&maskIt](std::uint64_t) { return !*(maskIt++); });

    auto const dtype = dtype_to_numpy(r.getDatatype());
    auto a = py::array(dtype, shape);

    load_chunk(r, a, offset, extent);

    return a;
}

void init_RecordComponent(py::module &m)
{
    py::class_<PythonDynamicMemoryView>(m, "Dynamic_Memory_View")
        .def(
            "__repr__",
            [](PythonDynamicMemoryView const &view) {
                return "<openPMD.Dynamic_Memory_view of dimensionality '" +
                    std::to_string(view.m_arrayShape->size()) + "'>";
            })
        .def("current_buffer", [](PythonDynamicMemoryView const &view) {
            return view.currentView();
        });

    py::class_<RecordComponent, BaseRecordComponent> cl(m, "Record_Component");
    cl.def(
          "__repr__",
          [](RecordComponent const &rc) {
              return "<openPMD.Record_Component of dimensionality '" +
                  std::to_string(rc.getDimensionality()) + "'>";
          })

        .def_property(
            "unit_SI",
            &BaseRecordComponent::unitSI,
            &RecordComponent::setUnitSI)

        .def("reset_dataset", &RecordComponent::resetDataset)

        .def_property_readonly("ndim", &RecordComponent::getDimensionality)
        .def_property_readonly("shape", &RecordComponent::getExtent)
        .def_property_readonly("empty", &RecordComponent::empty)

        // buffer types
        .def(
            "make_constant",
            [](RecordComponent &rc, py::buffer &a) {
                py::buffer_info buf = a.request();
                auto const dtype = dtype_from_bufferformat(buf.format);

                using DT = Datatype;

                // allow one-element n-dimensional buffers as well
                py::ssize_t numElements = 1;
                if (buf.ndim > 0)
                {
                    for (auto d = 0; d < buf.ndim; ++d)
                        numElements *= buf.shape.at(d);
                }

                // Numpy: Handling of arrays and scalars
                // work-around for
                // https://github.com/pybind/pybind11/issues/1224
                // -> passing numpy scalars as buffers needs numpy 1.15+
                //    https://github.com/numpy/numpy/issues/10265
                //    https://github.com/pybind/pybind11/issues/1224#issuecomment-354357392
                // scalars, see PEP 3118
                // requires Numpy 1.15+
                if (numElements == 1)
                {
                    // refs:
                    //   https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.interface.html
                    //   https://docs.python.org/3/library/struct.html#format-characters
                    // std::cout << "  scalar type '" << buf.format << "'" <<
                    // std::endl; typestring: encoding + type + number of bytes
                    switch (dtype)
                    {
                    case DT::BOOL:
                        return rc.makeConstant(*static_cast<bool *>(buf.ptr));
                        break;
                    case DT::CHAR:
                        return rc.makeConstant(*static_cast<char *>(buf.ptr));
                        break;
                    case DT::SHORT:
                        return rc.makeConstant(*static_cast<short *>(buf.ptr));
                        break;
                    case DT::INT:
                        return rc.makeConstant(*static_cast<int *>(buf.ptr));
                        break;
                    case DT::LONG:
                        return rc.makeConstant(*static_cast<long *>(buf.ptr));
                        break;
                    case DT::LONGLONG:
                        return rc.makeConstant(
                            *static_cast<long long *>(buf.ptr));
                        break;
                    case DT::UCHAR:
                        return rc.makeConstant(
                            *static_cast<unsigned char *>(buf.ptr));
                        break;
                    case DT::USHORT:
                        return rc.makeConstant(
                            *static_cast<unsigned short *>(buf.ptr));
                        break;
                    case DT::UINT:
                        return rc.makeConstant(
                            *static_cast<unsigned int *>(buf.ptr));
                        break;
                    case DT::ULONG:
                        return rc.makeConstant(
                            *static_cast<unsigned long *>(buf.ptr));
                        break;
                    case DT::ULONGLONG:
                        return rc.makeConstant(
                            *static_cast<unsigned long long *>(buf.ptr));
                        break;
                    case DT::FLOAT:
                        return rc.makeConstant(*static_cast<float *>(buf.ptr));
                        break;
                    case DT::DOUBLE:
                        return rc.makeConstant(*static_cast<double *>(buf.ptr));
                        break;
                    case DT::LONG_DOUBLE:
                        return rc.makeConstant(
                            *static_cast<long double *>(buf.ptr));
                        break;
                    case DT::CFLOAT:
                        return rc.makeConstant(
                            *static_cast<std::complex<float> *>(buf.ptr));
                        break;
                    case DT::CDOUBLE:
                        return rc.makeConstant(
                            *static_cast<std::complex<double> *>(buf.ptr));
                        break;
                    case DT::CLONG_DOUBLE:
                        return rc.makeConstant(
                            *static_cast<std::complex<long double> *>(buf.ptr));
                        break;
                    default:
                        throw std::runtime_error(
                            "make_constant: "
                            "Unknown Datatype!");
                    }
                }
                else
                {
                    throw std::runtime_error(
                        "make_constant: "
                        "Only scalar values supported!");
                }
            },
            py::arg("value"))
        // allowed python intrinsics, after (!) buffer matching
        .def(
            "make_constant",
            &RecordComponent::makeConstant<char>,
            py::arg("value"))
        .def(
            "make_constant",
            &RecordComponent::makeConstant<long>,
            py::arg("value"))
        .def(
            "make_constant",
            &RecordComponent::makeConstant<double>,
            py::arg("value"))
        .def(
            "make_constant",
            &RecordComponent::makeConstant<bool>,
            py::arg("value"))
        .def(
            "make_empty",
            [](RecordComponent &rc, Datatype dt, uint8_t dimensionality) {
                return rc.makeEmpty(dt, dimensionality);
            },
            py::arg("datatype"),
            py::arg("dimensionality"))
        .def(
            "make_empty",
            [](RecordComponent &rc,
               pybind11::dtype const dt,
               uint8_t dimensionality) {
                return rc.makeEmpty(dtype_from_numpy(dt), dimensionality);
            })

        // TODO if we also want to support scalar arrays, we have to switch
        //      py::array for py::buffer as in Attributable
        //      https://github.com/pybind/pybind11/pull/1537

        // slicing protocol
        .def(
            "__getitem__",
            [](RecordComponent &r, py::tuple const &slices) {
                return load_chunk(r, slices);
            },
            py::arg("tuple of index slices"))
        .def(
            "__getitem__",
            [](RecordComponent &r, py::slice const &slice_obj) {
                auto const slices = py::make_tuple(slice_obj);
                return load_chunk(r, slices);
            },
            py::arg("slice"))
        .def(
            "__getitem__",
            [](RecordComponent &r, py::int_ const &slice_obj) {
                auto const slices = py::make_tuple(slice_obj);
                return load_chunk(r, slices);
            },
            py::arg("axis index"))

        .def(
            "__setitem__",
            [](RecordComponent &r, py::tuple const &slices, py::array &a) {
                store_chunk(r, a, slices);
            },
            py::arg("tuple of index slices"),
            py::arg("array with values to assign"))
        .def(
            "__setitem__",
            [](RecordComponent &r, py::slice const &slice_obj, py::array &a) {
                auto const slices = py::make_tuple(slice_obj);
                store_chunk(r, a, slices);
            },
            py::arg("slice"),
            py::arg("array with values to assign"))
        .def(
            "__setitem__",
            [](RecordComponent &r, py::int_ const &slice_obj, py::array &a) {
                auto const slices = py::make_tuple(slice_obj);
                store_chunk(r, a, slices);
            },
            py::arg("axis index"),
            py::arg("array with values to assign"))

        // deprecated: pass-through C++ API
        .def(
            "load_chunk",
            [](RecordComponent &r,
               Offset const &offset_in,
               Extent const &extent_in) {
                uint8_t ndim = r.getDimensionality();

                // default arguments
                //   offset = {0u}: expand to right dim {0u, 0u, ...}
                Offset offset = offset_in;
                if (offset_in.size() == 1u && offset_in.at(0) == 0u)
                    offset = Offset(ndim, 0u);

                //   extent = {-1u}: take full size
                Extent extent(ndim, 1u);
                if (extent_in.size() == 1u && extent_in.at(0) == -1u)
                {
                    extent = r.getExtent();
                    for (uint8_t i = 0u; i < ndim; ++i)
                        extent[i] -= offset[i];
                }
                else
                    extent = extent_in;

                std::vector<ptrdiff_t> shape(extent.size());
                std::copy(
                    std::begin(extent), std::end(extent), std::begin(shape));
                auto const dtype = dtype_to_numpy(r.getDatatype());
                auto a = py::array(dtype, shape);
                load_chunk(r, a, offset, extent);

                return a;
            },
            py::arg_v(
                "offset", Offset(1, 0u), "np.zeros(Record_Component.shape)"),
            py::arg_v("extent", Extent(1, -1u), "Record_Component.shape"))
        .def(
            "load_chunk",
            [](RecordComponent &r,
               py::buffer buffer,
               Offset const &offset_in,
               Extent const &extent_in) {
                uint8_t ndim = r.getDimensionality();

                // default arguments
                //   offset = {0u}: expand to right dim {0u, 0u, ...}
                Offset offset = offset_in;
                if (offset_in.size() == 1u && offset_in.at(0) == 0u)
                    offset = Offset(ndim, 0u);

                //   extent = {-1u}: take full size
                Extent extent(ndim, 1u);
                if (extent_in.size() == 1u && extent_in.at(0) == -1u)
                {
                    extent = r.getExtent();
                    for (uint8_t i = 0u; i < ndim; ++i)
                        extent[i] -= offset[i];
                }
                else
                    extent = extent_in;

                std::vector<bool> flatten(ndim, false);
                load_chunk(r, buffer, offset, extent);
            },
            py::arg("pre-allocated buffer"),
            py::arg_v(
                "offset", Offset(1, 0u), "np.zeros(Record_Component.shape)"),
            py::arg_v("extent", Extent(1, -1u), "Record_Component.shape"))

        // deprecated: pass-through C++ API
        .def(
            "store_chunk",
            [](RecordComponent &r,
               py::array &a,
               Offset const &offset_in,
               Extent const &extent_in) {
                // default arguments
                //   offset = {0u}: expand to right dim {0u, 0u, ...}
                Offset offset = offset_in;
                if (offset_in.size() == 1u && offset_in.at(0) == 0u &&
                    a.ndim() > 1)
                    offset = Offset(a.ndim(), 0u);

                //   extent = {-1u}: take full size
                Extent extent(a.ndim(), 1u);
                if (extent_in.size() == 1u && extent_in.at(0) == -1u)
                    for (auto d = 0; d < a.ndim(); ++d)
                        extent.at(d) = a.shape()[d];
                else
                    extent = extent_in;

                std::vector<bool> flatten(r.getDimensionality(), false);
                store_chunk(r, a, offset, extent, flatten);
            },
            py::arg("array"),
            py::arg_v("offset", Offset(1, 0u), "np.zeros_like(array)"),
            py::arg_v("extent", Extent(1, -1u), "array.shape"))
        .def(
            "store_chunk",
            [](RecordComponent &r,
               Offset const &offset_in,
               Extent const &extent_in) {
                // default arguments
                //   offset = {0u}: expand to right dim {0u, 0u, ...}
                unsigned dimensionality = r.getDimensionality();
                Extent const &totalExtent = r.getExtent();
                Offset offset = offset_in;
                if (offset_in.size() == 1u && offset_in.at(0) == 0u &&
                    dimensionality > 1u)
                    offset = Offset(dimensionality, 0u);

                //   extent = {-1u}: take full size
                Extent extent(dimensionality, 1u);
                if (extent_in.size() == 1u && extent_in.at(0) == -1u)
                    for (unsigned d = 0; d < dimensionality; ++d)
                        extent.at(d) = totalExtent[d];
                else
                    extent = extent_in;

                std::vector<bool> flatten(r.getDimensionality(), false);
                return store_chunk_span(r, offset, extent, flatten);
            },
            py::arg_v("offset", Offset(1, 0u), "np.zeros_like(array)"),
            py::arg_v("extent", Extent(1, -1u), "array.shape"))

        .def_property_readonly_static(
            "SCALAR", [](py::object) { return RecordComponent::SCALAR; })

        // TODO remove in future versions (deprecated)
        .def("set_unit_SI", &RecordComponent::setUnitSI) // deprecated
        ;
    add_pickle(
        cl, [](openPMD::Series &series, std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            return series.iterations[n_it]
                .particles[group.at(3)][group.at(4)][group.at(5)];
        });

    py::enum_<RecordComponent::Allocation>(m, "Allocation")
        .value("USER", RecordComponent::Allocation::USER)
        .value("API", RecordComponent::Allocation::API)
        .value("AUTO", RecordComponent::Allocation::AUTO);
}
