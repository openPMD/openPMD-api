/* Copyright 2018-2025 Axel Huebl, Franz Poeschel, Luca Fedeli
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

#include "openPMD/RecordComponent.hpp"
#include "openPMD/Dataset.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/DatatypeHelpers.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include "openPMD/binding/python/Numpy.hpp"
#include "openPMD/binding/python/Pickle.hpp"
#include "openPMD/binding/python/RecordComponent.hpp"

#include <algorithm>
#include <complex>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

/** Same as pybind11::slice::compute() */
inline bool compute_slice(
    nb::handle slice,
    size_t length,
    size_t *start,
    size_t *stop,
    size_t *step,
    size_t *slicelength)
{
    return PySlice_GetIndicesEx(
               slice.ptr(),
               static_cast<Py_ssize_t>(length),
               reinterpret_cast<Py_ssize_t *>(start),
               reinterpret_cast<Py_ssize_t *>(stop),
               reinterpret_cast<Py_ssize_t *>(step),
               reinterpret_cast<Py_ssize_t *>(slicelength)) == 0;
}

/** Convert a nb::tuple of nb::slices to Offset & Extent
 *
 * https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.indexing.html
 * https://github.com/numpy/numpy/blob/v1.16.1/numpy/core/src/multiarray/mapping.c#L348-L375
 */
inline std::tuple<Offset, Extent, std::vector<bool>> parseTupleSlices(
    uint8_t const ndim, Extent const &full_extent, nb::tuple const &slices)
{
    uint8_t const numSlices = nb::len(slices);

    Offset offset(ndim, 0u);
    Extent extent(ndim, 1u);
    std::vector<bool> flatten(ndim, false);
    int16_t curAxis = -1;

    int16_t posEllipsis = -1;
    for (uint8_t i = 0u; i < numSlices; ++i)
    {
        ++curAxis;

        if (i >= ndim && posEllipsis == -1 && slices[i].ptr() != Py_Ellipsis)
            throw nb::index_error(
                "too many indices for dimension of record component!");

        if (slices[i].ptr() == Py_Ellipsis)
        {
            // only allowed once
            if (posEllipsis != -1)
                throw nb::index_error(
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
            nb::object slice = slices[i];

            size_t start, stop, step, slicelength;
            auto mocked_extent = full_extent.at(curAxis);
            // Py_ssize_t is a signed type, so we will need to use another
            // magic number for JOINED_DIMENSION in this computation, since the
            // C++ API's JOINED_DIMENSION would be interpreted as a negative
            // index
            bool undo_mocked_extent = false;
            constexpr auto PYTHON_JOINED_DIMENSION =
                std::numeric_limits<Py_ssize_t>::max() - 1;
            if (mocked_extent == Dataset::JOINED_DIMENSION)
            {
                undo_mocked_extent = true;
                mocked_extent = PYTHON_JOINED_DIMENSION;
            }
            if (!compute_slice(
                    slice, mocked_extent, &start, &stop, &step, &slicelength))
                throw nb::python_error();

            if (undo_mocked_extent)
            {
                // do the same calculation again, but with another global extent
                // (that is not smaller than the previous in order to avoid
                // cutting off the range)
                // this is to avoid the unlikely case
                // that the mocked alternative value is actually the intended
                // one
                size_t start2, stop2, step2, slicelength2;
                if (!compute_slice(
                        slice,
                        mocked_extent + 1,
                        &start2,
                        &stop2,
                        &step2,
                        &slicelength2))
                    throw nb::python_error();
                if (slicelength == slicelength2)
                {
                    // slicelength was given as an absolute value and
                    // accidentally hit our mocked value
                    // --> keep that value
                    undo_mocked_extent = false;
                }
            }

            // TODO PySlice_AdjustIndices: Python 3.6.1+
            //      Adjust start/end slice indices assuming a sequence of the
            //      specified length. Out of bounds indices are clipped in a
            //      manner consistent with the handling of normal slices.
            // slicelength = PySlice_AdjustIndices(full_extent[curAxis],
            // (ssize_t*)&start, (ssize_t*)&stop, step);

            if (step != 1u)
                throw nb::index_error(
                    "strides in selection are inefficient, not implemented!");

            // verified for size later in C++ API
            offset.at(curAxis) = start;
            extent.at(curAxis) =
                undo_mocked_extent && slicelength == PYTHON_JOINED_DIMENSION
                ? Dataset::JOINED_DIMENSION
                : slicelength; // stop - start;

            continue;
        }

        try
        {
            auto const index = nb::cast<std::int64_t>(slices[i]);

            if (index < 0)
                offset.at(curAxis) = full_extent.at(curAxis) + index;
            else
                offset.at(curAxis) = index;

            extent.at(curAxis) = 1;
            flatten.at(curAxis) = true; // indices flatten the dimension

            if (offset.at(curAxis) >= full_extent.at(curAxis))
                throw nb::index_error(
                    (std::string("index ") +
                     std::to_string(offset.at(curAxis)) +
                     std::string(" is out of bounds for axis ") +
                     std::to_string(i) + std::string(" with size ") +
                     std::to_string(full_extent.at(curAxis)))
                        .c_str());

            continue;
        }
        // NOLINTNEXTLINE(bugprone-empty-catch)
        catch (const nb::cast_error &e)
        {
            // not an index
        }

        if (slices[i].ptr() == Py_None)
        {
            // nb::none newaxis = nb::cast< nb::none >( slices[i] );;
            throw nb::index_error("None (newaxis) not implemented!");

            // continue;
        }

        // if we get here, the last slice type was not successfully processed
        --curAxis;
        throw nb::index_error((std::string("unknown index type passed: ") +
                               nb::cast<std::string>(nb::str(slices[i])))
                                  .c_str());
    }

    // fill omitted higher indices with "select all"
    for (++curAxis; curAxis < int16_t(ndim); ++curAxis)
    {
        extent.at(curAxis) = full_extent.at(curAxis);
    }

    return std::make_tuple(offset, extent, flatten);
}

inline std::tuple<Offset, Extent, std::vector<bool>> parseJoinedTupleSlices(
    uint8_t const ndim,
    Extent const &full_extent,
    nb::tuple const &slices,
    size_t joined_dim,
    python::array const &a)
{

    std::vector<bool> flatten;
    Offset offset;
    Extent extent;
    std::tie(offset, extent, flatten) =
        parseTupleSlices(ndim, full_extent, slices);
    for (size_t i = 0; i < ndim; ++i)
    {
        if (offset.at(i) != 0)
        {
            throw std::runtime_error(
                "Joined array: Cannot use non-zero offset in store_chunk "
                "(offset[" +
                std::to_string(i) + "] = " + std::to_string(offset[i]) + ").");
        }
        if (flatten.at(i))
        {
            throw std::runtime_error(
                "Flattened slices unimplemented for joined arrays.");
        }

        if (i == joined_dim)
        {
            if (extent.at(i) == 0 || extent.at(i) == Dataset::JOINED_DIMENSION)
            {
                extent[i] = a.shape().at(i);
            }
        }
        else
        {
            if (extent.at(i) != full_extent.at(i))
            {
                throw std::runtime_error(
                    "Joined array: Must use full extent in store_chunk for "
                    "non-joined dimension "
                    "(local_extent[" +
                    std::to_string(i) + "] = " + std::to_string(extent[i]) +
                    " != global_extent[" + std::to_string(i) +
                    "] = " + std::to_string(full_extent[i]) + ").");
            }
        }
    }
    offset.clear();
    return std::make_tuple(offset, extent, flatten);
}

/** Check an array is a contiguous buffer
 *
 * Required are contiguous buffers for store and load
 *
 * - not strided with paddings
 * - not a view in another buffer that results in striding
 */
inline void check_buffer_is_contiguous(python::array &a)
{

    if (!python::buffer_info(a).c_contiguous)
        throw nb::index_error(
            "strides in chunk are inefficient, not implemented!");
    // @todo in order to implement stride handling, one needs to
    //       loop over the input data strides in store/load calls
}

namespace
{
struct StoreChunkFromPythonArray
{
    template <typename T>
    static void call(
        RecordComponent &r,
        python::array &a,
        Offset const &offset,
        Extent const &extent)
    {
        void *data = a.mutable_data();
        // here, we store an owning handle in the lambda capture so that
        // temporary and lost-scope variables stay alive until we flush
        // note: this does not yet prevent the user, as in C++, to build
        // a race condition by manipulating the data that was passed
        std::shared_ptr<T> shared(
            (T *)data,
            [owning_handle = std::make_optional(nb::object(a))](T *) mutable {
                nb::gil_scoped_acquire need_the_gil_for_this;
                owning_handle.reset();
            });
        r.storeChunk(std::move(shared), offset, extent);
    }

    static constexpr char const *errorMsg = "store_chunk()";
};
struct LoadChunkIntoPythonArray
{
    template <typename T>
    static void call(
        RecordComponent &r,
        python::array &a,
        Offset const &offset,
        Extent const &extent)
    {
        void *data = a.mutable_data();
        // here, we store an owning handle in the lambda capture so that
        // temporary and lost-scope variables stay alive until we flush
        // note: this does not yet prevent the user, as in C++, to build
        // a race condition by manipulating the data that was passed
        std::shared_ptr<T> shared(
            (T *)data,
            [owning_handle = std::make_optional(nb::object(a))](T *) mutable {
                nb::gil_scoped_acquire need_the_gil_for_this;
                owning_handle.reset();
            });
        r.loadChunk(std::move(shared), offset, extent);
    }

    static constexpr char const *errorMsg = "load_chunk()";
};
struct LoadChunkIntoPythonBuffer
{
    template <typename T>
    static void call(
        RecordComponent &r,
        python::buffer &buffer,
        python::buffer_info const &buffer_info,
        Offset const &offset,
        Extent const &extent)
    {
        void *data = buffer_info.ptr;
        // here, we store an owning handle in the lambda capture so that
        // temporary and lost-scope variables stay alive until we flush
        // note: this does not yet prevent the user, as in C++, to build
        // a race condition by manipulating the data that was passed
        std::shared_ptr<T> shared(
            (T *)data,
            [owning_handle =
                 std::make_optional(nb::object(buffer))](T *) mutable {
                nb::gil_scoped_acquire need_the_gil_for_this;
                owning_handle.reset();
            });
        r.loadChunk(std::move(shared), offset, extent);
    }

    static constexpr char const *errorMsg = "load_chunk()";
};
} // namespace

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
    python::array &a,
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
    auto const &s_extent(extent); // selected extent in r
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
    auto const a_shape = a.shape();
    if (size_t(a.ndim()) != r_shape.size())
        throw nb::index_error(
            (std::string("dimension of chunk (") + std::to_string(a.ndim()) +
             std::string(
                 "D) does not fit dimension of selection "
                 "in record component (") +
             std::to_string(r_shape.size()) + std::string("D)"))
                .c_str());

    if (auto joined_dim = r.joinedDimension(); joined_dim.has_value())
    {
        for (Py_ssize_t d = 0; d < a.ndim(); ++d)
        {
            // selection causes overflow of r
            if (d != Py_ssize_t(*joined_dim) && extent.at(d) != r_shape.at(d))
                throw nb::index_error(
                    (std::string("selection for axis ") + std::to_string(d) +
                     " of record component with joined dimension " +
                     std::to_string(*joined_dim) +
                     " must be equivalent to its global extent " +
                     std::to_string(extent.at(d)) + ", but was " +
                     std::to_string(r_shape.at(d)) + ".")
                        .c_str());
            // underflow of selection in r for given a
            if (s_shape.at(d) != std::uint64_t(a_shape[d]))
                throw nb::index_error(
                    (std::string("size of chunk (") +
                     std::to_string(a_shape[d]) + std::string(") for axis ") +
                     std::to_string(d) +
                     std::string(" does not match selection ") +
                     std::string("size in record component (") +
                     std::to_string(s_extent.at(d)) + std::string(")"))
                        .c_str());
        }
    }
    else
    {
        for (auto d = 0; d < a.ndim(); ++d)
        {
            // selection causes overflow of r
            if (offset.at(d) + extent.at(d) > r_shape.at(d))
                throw nb::index_error(
                    (std::string("slice ") + std::to_string(offset.at(d)) +
                     std::string(":") + std::to_string(extent.at(d)) +
                     std::string(" is out of bounds for axis ") +
                     std::to_string(d) + std::string(" with size ") +
                     std::to_string(r_shape.at(d)))
                        .c_str());
            // underflow of selection in r for given a
            if (s_shape.at(d) != std::uint64_t(a_shape[d]))
                throw nb::index_error(
                    (std::string("size of chunk (") +
                     std::to_string(a_shape[d]) + std::string(") for axis ") +
                     std::to_string(d) +
                     std::string(" does not match selection ") +
                     std::string("size in record component (") +
                     std::to_string(s_extent.at(d)) + std::string(")"))
                        .c_str());
        }
    }

    check_buffer_is_contiguous(a);

    if (!dtype_to_numpy(r.getDatatype()).is(a.dtype()))
    {
        std::stringstream err;
        err << "Attempting store from Python array of type '"
            << dtype_from_numpy(a.dtype())
            << "' into Record Component of type '" << r.getDatatype() << "'.";
        throw error::WrongAPIUsage(err.str());
    }
    switchDatasetType<StoreChunkFromPythonArray>(
        r.getDatatype(), r, a, offset, extent);
}

/** Store Chunk
 *
 * Called with a nb::tuple of slices and a python::array
 */
inline void
store_chunk(RecordComponent &r, python::array &a, nb::tuple const &slices)
{
    uint8_t ndim = r.getDimensionality();
    auto const full_extent = r.getExtent();

    Offset offset;
    Extent extent;
    std::vector<bool> flatten;
    if (auto joined_dimension = r.joinedDimension();
        joined_dimension.has_value())
    {
        std::tie(offset, extent, flatten) = parseJoinedTupleSlices(
            ndim, full_extent, slices, *joined_dimension, a);
    }
    else
    {
        std::tie(offset, extent, flatten) =
            parseTupleSlices(ndim, full_extent, slices);
    }

    store_chunk(r, a, offset, extent, flatten);
}

struct PythonDynamicMemoryView
{
    using ShapeContainer = std::vector<Py_ssize_t>;

    template <typename T>
    PythonDynamicMemoryView(
        DynamicMemoryView<T> dynamicView,
        ShapeContainer arrayShape,
        ShapeContainer strides)
        : m_dynamicView(
              std::shared_ptr<void>(
                  new DynamicMemoryView<T>(std::move(dynamicView))))
        , m_arrayShape(std::move(arrayShape))
        , m_strides(std::move(strides))
        , m_datatype(determineDatatype<T>())
    {}

    [[nodiscard]] nb::object currentView() const;

    std::shared_ptr<void> m_dynamicView;
    ShapeContainer m_arrayShape;
    ShapeContainer m_strides;
    Datatype m_datatype;
};

namespace
{
/** PEP 3118 format string for T, same as pybind11::format_descriptor<T> */
template <typename T>
char const *buffer_format()
{
    if constexpr (std::is_same_v<T, bool>)
        return "?";
    else if constexpr (std::is_integral_v<T>)
    {
        constexpr bool is_signed = std::is_signed_v<T>;
        if constexpr (sizeof(T) == 1)
            return is_signed ? "b" : "B";
        else if constexpr (sizeof(T) == 2)
            return is_signed ? "h" : "H";
        else if constexpr (sizeof(T) == 4)
            return is_signed ? "i" : "I";
        else
            return is_signed ? "q" : "Q";
    }
    else if constexpr (std::is_same_v<T, float>)
        return "f";
    else if constexpr (std::is_same_v<T, double>)
        return "d";
    else if constexpr (std::is_same_v<T, long double>)
        return "g";
    else if constexpr (std::is_same_v<T, std::complex<float>>)
        return "Zf";
    else if constexpr (std::is_same_v<T, std::complex<double>>)
        return "Zd";
    else if constexpr (std::is_same_v<T, std::complex<long double>>)
        return "Zg";
    else
        static_assert(
            openPMD::auxiliary::dependent_false_v<T>,
            "Unsupported buffer type");
}

/** Same as pybind11::memoryview::from_buffer() */
template <typename T>
nb::object memoryview_from_buffer(
    T *ptr,
    std::vector<Py_ssize_t> const &shape,
    std::vector<Py_ssize_t> const &strides,
    bool readonly)
{
    Py_buffer view{};
    view.buf = ptr;
    view.obj = nullptr;
    view.itemsize = sizeof(T);
    view.len = view.itemsize;
    for (auto extent : shape)
    {
        view.len *= extent;
    }
    view.readonly = readonly ? 1 : 0;
    view.ndim = static_cast<int>(shape.size());
    view.format = const_cast<char *>(buffer_format<T>());
    // copied by PyMemoryView_FromBuffer
    view.shape = const_cast<Py_ssize_t *>(shape.data());
    view.strides = const_cast<Py_ssize_t *>(strides.data());
    PyObject *res = PyMemoryView_FromBuffer(&view);
    if (!res)
    {
        throw nb::python_error();
    }
    return nb::steal(res);
}

struct GetCurrentView
{
    template <typename T>
    static nb::object call(PythonDynamicMemoryView const &dynamicView)
    {
        auto span =
            static_cast<DynamicMemoryView<T> *>(dynamicView.m_dynamicView.get())
                ->currentBuffer();
        if (!span.data())
        {
            /*
             * Fallback for zero-sized store_chunk calls.
             * A memoryview cannot be used since it checks for nullpointers,
             * even when the extent is zero.
             * This may sound like an esoteric use case at first, but may happen
             * in parallel usage when the chunk distribution ends up assigning
             * zero-sized chunks to some rank.
             */
            return python::array(
                dtype_to_numpy(dynamicView.m_datatype),
                dynamicView.m_arrayShape);
        }
        else
        {
            return memoryview_from_buffer(
                span.data(),
                dynamicView.m_arrayShape,
                dynamicView.m_strides,
                /* readonly = */ false);
        }
    }

    static constexpr char const *errorMsg = "DynamicMemoryView";
};
} // namespace

nb::object PythonDynamicMemoryView::currentView() const
{
    return switchDatasetType<GetCurrentView>(m_datatype, *this);
}

namespace
{
struct StoreChunkSpan
{
    template <typename T>
    static PythonDynamicMemoryView
    call(RecordComponent &r, Offset const &offset, Extent const &extent)
    {
        DynamicMemoryView<T> dynamicView = r.storeChunk<T>(offset, extent);
        std::vector<Py_ssize_t> arrayShape(extent.begin(), extent.end());
        std::vector<Py_ssize_t> strides(extent.size());
        {
            Py_ssize_t accumulator = sizeof(T);
            size_t dim = extent.size();
            while (dim > 0)
            {
                --dim;
                strides[dim] = accumulator;
                accumulator *= extent[dim];
            }
        }
        return PythonDynamicMemoryView(
            std::move(dynamicView), std::move(arrayShape), std::move(strides));
    }

    static constexpr char const *errorMsg = "RecordComponent.store_chunk()";
};
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

    return switchDatasetType<StoreChunkSpan>(
        r.getDatatype(), r, offset, extent);
}

inline PythonDynamicMemoryView
store_chunk_span(RecordComponent &r, nb::tuple const &slices)
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
    python::buffer &buffer,
    Offset const &offset,
    Extent const &extent)
{
    auto const dtype = dtype_to_numpy(r.getDatatype());
    python::buffer_info buffer_info(buffer, /* writable = */ true);

    auto const &strides = buffer_info.strides;
    // this function requires a contiguous slab of memory, so check the strides
    // whether we have that
    if (strides.size() == 0)
    {
        throw error::WrongAPIUsage(
            "[Record_Component::load_chunk()] Empty buffer passed.");
    }
    {
        Py_ssize_t accumulator = toBytes(r.getDatatype());
        if (buffer_info.itemsize != accumulator)
        {
            std::stringstream errorMsg;
            errorMsg << "[Record_Component::load_chunk()] Loading from a "
                        "record component of type "
                     << r.getDatatype() << " with item size " << accumulator
                     << ", but Python buffer has item size "
                     << buffer_info.itemsize << ".";
            throw error::WrongAPIUsage(errorMsg.str());
        }
        size_t dim = strides.size();
        while (dim > 0)
        {
            --dim;
            if (strides[dim] != accumulator)
            {
                throw error::WrongAPIUsage(
                    "[Record_Component::load_chunk()] Requires contiguous slab"
                    " of memory.");
            }
            accumulator *= extent[dim];
        }
    }

    switchDatasetType<LoadChunkIntoPythonBuffer>(
        r.getDatatype(), r, buffer, buffer_info, offset, extent);
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
    python::array &a,
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
    auto const a_shape = a.shape();
    for (Py_ssize_t d = 0; d < a.ndim(); ++d)
    {
        s_array *= a_shape[d];
        str_array_shape.append(" ").append(std::to_string(a_shape[d]));
    }

    /* we allow flattening of the result dimension
    if( size_t(a.ndim()) > extent.size() )
        throw nb::index_error((
            std::string("dimension of array (") +
            std::to_string(a.ndim()) +
            std::string("D) does not fit dimension of selection "
                        "in record component (") +
            std::to_string(extent.size()) +
            std::string("D)")
        ).c_str());
    */
    if (s_array < s_load)
    {
        throw nb::index_error(
            (std::string("size of array (") + std::to_string(s_array) +
             std::string("; shape:") + str_array_shape +
             std::string(
                 ") is smaller than size of selection "
                 "in record component (") +
             std::to_string(s_load) + std::string("; shape:") +
             str_extent_shape + std::string(")"))
                .c_str());
    }

    check_buffer_is_contiguous(a);

    if (!dtype_to_numpy(r.getDatatype()).is(a.dtype()))
    {
        std::stringstream err;
        err << "Attempting load into Python array of type '"
            << dtype_from_numpy(a.dtype())
            << "' from Record Component of type '" << r.getDatatype() << "'.";
        throw error::WrongAPIUsage(err.str());
    }

    switchDatasetType<LoadChunkIntoPythonArray>(
        r.getDatatype(), r, a, offset, extent);
}

/** Load Chunk
 *
 * Called with a nb::tuple of slices.
 */
python::array load_chunk(RecordComponent &r, nb::tuple const &slices)
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
    auto a = python::array(dtype, shape);

    load_chunk(r, a, offset, extent);

    return a;
}

void init_RecordComponent(nb::module_ &m)
{
    using python::Self;
    using python::self_method;

    nb::class_<PythonDynamicMemoryView>(m, "Dynamic_Memory_View")
        .def(
            "__repr__",
            [](PythonDynamicMemoryView const &view) {
                return "<openPMD.Dynamic_Memory_view of dimensionality '" +
                    std::to_string(view.m_arrayShape.size()) + "'>";
            })
        .def("current_buffer", [](PythonDynamicMemoryView const &view) {
            return view.currentView();
        });

    auto py_rc_cnt =
        declare_container<PyRecordComponentContainer, Attributable>(
            m, "Record_Component_Container");

    auto cl = python::bind_class<RecordComponent, BaseRecordComponent>(
        m, "Record_Component");
    cl.def(
          "__repr__",
          [](Self<RecordComponent> self) {
              RecordComponent const &rc = *self;
              std::stringstream stream;
              stream << "<openPMD.Record_Component of type '"
                     << rc.getDatatype() << "' and with extent ";
              if (auto extent = rc.getExtent(); extent.empty())
              {
                  stream << "[]>";
              }
              else
              {
                  auto begin = extent.begin();
                  stream << '[' << *begin++;
                  for (; begin != extent.end(); ++begin)
                  {
                      stream << ", " << *begin;
                  }
                  stream << "]>";
              }
              return stream.str();
          })

        .def_prop_rw(
            "unit_SI",
            self_method<RecordComponent>(&BaseRecordComponent::unitSI),
            self_method(&RecordComponent::setUnitSI))

        .def("reset_dataset", self_method(&RecordComponent::resetDataset))

        .def_prop_ro("ndim", self_method(&RecordComponent::getDimensionality))
        .def_prop_ro("shape", self_method(&RecordComponent::getExtent))
        .def_prop_ro("empty", self_method(&RecordComponent::empty))

        // buffer types
        .def(
            "make_constant",
            [](Self<RecordComponent> self, python::buffer &a) {
                RecordComponent &rc = *self;
                python::buffer_info buf(a);
                auto const dtype = dtype_from_bufferformat(buf.format);

                using DT = Datatype;

                // allow one-element n-dimensional buffers as well
                Py_ssize_t numElements = 1;
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
                    case DT::BOOL:
                        throw std::runtime_error(
                            "make_constant: "
                            "Boolean type not supported!");
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
            nb::arg("value"))
        // allowed python intrinsics, after (!) buffer matching
        .def(
            "make_constant",
            self_method(&RecordComponent::makeConstant<char>),
            nb::arg("value"))
        .def(
            "make_constant",
            self_method(&RecordComponent::makeConstant<long>),
            nb::arg("value"))
        .def(
            "make_constant",
            self_method(&RecordComponent::makeConstant<double>),
            nb::arg("value"))
        .def(
            "make_empty",
            [](Self<RecordComponent> rc, Datatype dt, uint8_t dimensionality) {
                return rc->makeEmpty(dt, dimensionality);
            },
            nb::arg("datatype"),
            nb::arg("dimensionality"))
        .def(
            "make_empty",
            [](Self<RecordComponent> rc,
               nb::object dt,
               uint8_t dimensionality) {
                return rc->makeEmpty(
                    dtype_from_numpy(std::move(dt)), dimensionality);
            })

        // deprecated: pass-through C++ API
        .def(
            "load_chunk",
            [](Self<RecordComponent> self,
               Offset const &offset_in,
               Extent const &extent_in) {
                RecordComponent &r = *self;
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
                auto a = python::array(dtype, shape);
                load_chunk(r, a, offset, extent);

                return a;
            },
            nb::arg("offset").sig("np.zeros(Record_Component.shape)") =
                Offset(1, 0u),
            nb::arg("extent").sig("Record_Component.shape") = Extent(1, -1u))
        .def(
            "load_chunk",
            [](Self<RecordComponent> self,
               python::buffer buffer,
               Offset const &offset_in,
               Extent const &extent_in) {
                RecordComponent &r = *self;
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
            nb::arg("pre-allocated buffer"),
            nb::arg("offset").sig("np.zeros(Record_Component.shape)") =
                Offset(1, 0u),
            nb::arg("extent").sig("Record_Component.shape") = Extent(1, -1u))

        // deprecated: pass-through C++ API
        .def(
            "store_chunk",
            [](Self<RecordComponent> self,
               python::array &a,
               Offset const &offset_in,
               Extent const &extent_in) {
                RecordComponent &r = *self;
                // default arguments
                //   offset = {0u}: expand to right dim {0u, 0u, ...}
                Offset offset = offset_in;
                if (offset_in.size() == 1u && offset_in.at(0) == 0u &&
                    a.ndim() > 1)
                    offset = Offset(a.ndim(), 0u);

                //   extent = {-1u}: take full size
                Extent extent(a.ndim(), 1u);
                if (extent_in.size() == 1u && extent_in.at(0) == -1u)
                {
                    auto const a_shape = a.shape();
                    for (auto d = 0; d < a.ndim(); ++d)
                        extent.at(d) = a_shape[d];
                }
                else
                    extent = extent_in;

                std::vector<bool> flatten(r.getDimensionality(), false);
                store_chunk(r, a, offset, extent, flatten);
            },
            nb::arg("array"),
            nb::arg("offset").sig("np.zeros_like(array)") = Offset(1, 0u),
            nb::arg("extent").sig("array.shape") = Extent(1, -1u))
        .def(
            "store_chunk",
            [](Self<RecordComponent> self,
               Offset const &offset_in,
               Extent const &extent_in) {
                RecordComponent &r = *self;
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
            nb::arg("offset").sig("np.zeros_like(array)") = Offset(1, 0u),
            nb::arg("extent").sig("array.shape") = Extent(1, -1u))

        .def_prop_ro_static(
            "SCALAR",
            [](nb::object const &) { return RecordComponent::SCALAR; })

        // TODO remove in future versions (deprecated)
        .def(
            "set_unit_SI",
            self_method(&RecordComponent::setUnitSI)) // deprecated
        ;
    add_pickle(
        cl,
        [](std::shared_ptr<openPMD::Series> series,
           std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            auto res = series->iterations[n_it]
                           .open()
                           .particles[group.at(3)][group.at(4)]
                                     [group.size() < 6 ? RecordComponent::SCALAR
                                                       : group.at(5)];
            return internal::makeOwning(res, std::move(series));
        });

    addRecordComponentSetGet(cl);

    finalize_container<PyRecordComponentContainer>(py_rc_cnt);
    auto py_base_record = addRecordComponentSetGet(
                              finalize_container<PyBaseRecordRecordComponent>(
                                  declare_container<
                                      PyBaseRecordRecordComponent,
                                      PyRecordComponentContainer>(
                                      m, "Base_Record_Record_Component")))
                              .def_prop_ro(
                                  "scalar",
                                  &BaseRecord<RecordComponent>::scalar,
                                  &docstring::is_scalar[1]);
    // BaseRecord<RecordComponent> also is a RecordComponent
    python::inherit_attributes(py_base_record, cl, nb::type<Attributable>());

    nb::enum_<RecordComponent::Allocation>(m, "Allocation")
        .value("USER", RecordComponent::Allocation::USER)
        .value("API", RecordComponent::Allocation::API)
        .value("AUTO", RecordComponent::Allocation::AUTO);
}
