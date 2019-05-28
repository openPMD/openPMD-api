/* Copyright 2018-2019 Axel Huebl
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
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/auxiliary/ShareRaw.hpp"
#include "openPMD/binding/python/Numpy.hpp"

#include <string>
#include <algorithm>
#include <tuple>
#include <iostream>
#include <cstdint>
#include <exception>

namespace py = pybind11;
using namespace openPMD;


/** Convert a py::tuple of py::slices to Offset & Extent
 *
 * https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.indexing.html
 * https://github.com/numpy/numpy/blob/v1.16.1/numpy/core/src/multiarray/mapping.c#L348-L375
 */
inline std::tuple< Offset, Extent, std::vector<bool> >
parseTupleSlices(uint8_t const ndim, Extent const & full_extent, py::tuple const & slices) {
    uint8_t const numSlices = py::len(slices);

    Offset offset(ndim, 0u);
    Extent extent(ndim, 1u);
    std::vector<bool> flatten(ndim, false);
    int16_t curAxis = -1;

    int16_t posEllipsis = -1;
    for( uint8_t i = 0u; i < numSlices; ++i )
    {
        ++curAxis;

        if(
            i >= ndim &&
            posEllipsis == -1 &&
            slices[i].ptr() != Py_Ellipsis
        )
            throw py::index_error(
                "too many indices for dimension of record component!");

        if( slices[i].ptr() == Py_Ellipsis )
        {
            // only allowed once
            if( posEllipsis != -1 )
                throw py::index_error("an index can only have a single ellipsis ('...')");
            posEllipsis = curAxis;

            // might be omitted if all other indices are given as well
            if( numSlices == ndim + 1 )
            {
                --curAxis;
                continue;
            }

            // how many slices were given after the ellipsis
            uint8_t const numSlicesAfterEllipsis =
                numSlices -
                uint8_t(posEllipsis) -
                1u;
            // how many slices does the ellipsis represent
            uint8_t const numSlicesEllipsis =
                numSlices
                - uint8_t(posEllipsis)     // slices before
                - numSlicesAfterEllipsis;  // slices after

            // fill ellipsis indices
            // note: if enough further indices are given, the ellipsis
            //       might stand for no axis: valid and ignored
            for( ; curAxis < posEllipsis + int16_t(numSlicesEllipsis); ++curAxis )
            {
                offset.at(curAxis) = 0;
                extent.at(curAxis) = full_extent.at(curAxis);
            }
            --curAxis;

            continue;
        }

        if( PySlice_Check( slices[i].ptr() ) )
        {
            py::slice slice = py::cast< py::slice >( slices[i] );

            size_t start, stop, step, slicelength;
            if( !slice.compute( full_extent.at(curAxis), &start, &stop, &step, &slicelength ) )
                throw py::error_already_set();

            // TODO PySlice_AdjustIndices: Python 3.6.1+
            //      Adjust start/end slice indices assuming a sequence of the specified length.
            //      Out of bounds indices are clipped in a manner consistent with the handling of normal slices.
            // slicelength = PySlice_AdjustIndices(full_extent[curAxis], (ssize_t*)&start, (ssize_t*)&stop, step);

            if( step != 1u )
                throw py::index_error("strides in selection are inefficient, not implemented!");

            // verified for size later in C++ API
            offset.at(curAxis) = start;
            extent.at(curAxis) = slicelength; // stop - start;

            continue;
        }

        try
        {
            auto const index = py::cast< std::int64_t >( slices[i] );

            if( index < 0 )
                offset.at(curAxis) = full_extent.at(curAxis) + index;
            else
                offset.at(curAxis) = index;

            extent.at(curAxis) = 1;
            flatten.at(curAxis) = true; // indices flatten the dimension

            if( offset.at(curAxis) >= full_extent.at(curAxis) )
                throw py::index_error(
                    std::string("index ") +
                    std::to_string( offset.at(curAxis) ) +
                    std::string(" is out of bounds for axis ") +
                    std::to_string(i) +
                    std::string(" with size ") +
                    std::to_string(full_extent.at(curAxis))
                );

            continue;
        }
        catch (const py::cast_error& e) {
            // not an index
        }

        if( slices[i].ptr() == Py_None )
        {
            py::none newaxis = py::cast< py::none >( slices[i] );;
            throw py::index_error("None (newaxis) not implemented!");

            continue;
        }

        // if we get here, the last slice type was not successfully processed
        --curAxis;
        throw py::index_error(
            std::string("unknown index type passed: ") +
            py::str(slices[i]).cast<std::string>());
    }

    // fill omitted higher indices with "select all"
    for( ++curAxis; curAxis < int16_t(ndim); ++curAxis )
    {
        extent.at(curAxis) = full_extent.at(curAxis);
    }

    return std::make_tuple(offset, extent, flatten);
}

/** Store Chunk
 *
 * Called with offset and extent that are already in the record component's
 * dimension.
 *
 * Size checks of the requested chunk (spanned data is in valid bounds)
 * will be performed at C++ API part in RecordComponent::storeChunk .
 */
inline void
store_chunk(RecordComponent & r, py::array & a, Offset const & offset, Extent const & extent, std::vector<bool> const & flatten) {
    // @todo keep locked until flush() is performed
    // a.flags.writable = false;
    // a.flags.owndata = false;

    // verify offset + extend fit in dataset extent

    //   some one-size dimensions might be flattended in our r due to selections by index
    size_t const numFlattenDims = std::count(flatten.begin(), flatten.end(), true);
    auto const r_extent = r.getExtent();
    auto const s_extent(extent);  // selected extent in r
    std::vector< std::uint64_t > r_shape(r_extent.size() - numFlattenDims);
    std::vector< std::uint64_t > s_shape(s_extent.size() - numFlattenDims);
    auto maskIt = flatten.begin();
    std::copy_if(
        std::begin(r_extent),
        std::end(r_extent),
        std::begin(r_shape),
        [&maskIt](std::uint64_t){
            return !*(maskIt++);
        }
    );
    maskIt = flatten.begin();
    std::copy_if(
        std::begin(s_extent),
        std::end(s_extent),
        std::begin(s_shape),
        [&maskIt](std::uint64_t){
            return !*(maskIt++);
        }
    );

    //   verify shape and extent
    if( size_t(a.ndim()) != r_shape.size() )
        throw py::index_error(
            std::string("dimension of chunk (") +
            std::to_string(a.ndim()) +
            std::string("D) does not fit dimension of selection "
                        "in record component (") +
            std::to_string(r_shape.size()) +
            std::string("D)")
        );

    for( auto d = 0; d < a.ndim(); ++d )
    {
        // selection causes overflow of r
        if( offset.at(d) + extent.at(d) > r_shape.at(d) )
            throw py::index_error(
                std::string("slice ") +
                std::to_string( offset.at(d) ) +
                std::string(":") +
                std::to_string( extent.at(d) ) +
                std::string(" is out of bounds for axis ") +
                std::to_string(d) +
                std::string(" with size ") +
                std::to_string(r_shape.at(d))
            );
        // underflow of selection in r for given a
        if( s_shape.at(d) != std::uint64_t(a.shape()[d]) )
            throw py::index_error(
                std::string("size of chunk (") +
                std::to_string( a.shape()[d] ) +
                std::string(") for axis ") +
                std::to_string(d) +
                std::string(" does not match selection ") +
                std::string("size in record component (") +
                std::to_string( s_extent.at(d) ) +
                std::string(")")
            );
    }

    /* required are contiguous buffers
     *
     * - not strided with paddings
     * - not a view in another buffer that results in striding
     */
    Py_buffer* view = new Py_buffer();
    int flags = PyBUF_STRIDES | PyBUF_FORMAT;
    if( PyObject_GetBuffer( a.ptr(), view, flags ) != 0 )
    {
        delete view;
        throw py::error_already_set();
    }
    bool isContiguous = ( PyBuffer_IsContiguous( view, 'A' ) != 0 );
    PyBuffer_Release( view );
    delete view;

    if( !isContiguous )
        throw py::index_error(
            "strides in chunk are inefficient, not implemented!");
    // @todo in order to implement stride handling, one needs to
    //       loop over the input data strides during write below,
    //       also data might not be owned!

    // store
    auto const dtype = dtype_from_numpy( a.dtype() );
    if( dtype == Datatype::CHAR )
        r.storeChunk( shareRaw( (char*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::UCHAR )
        r.storeChunk( shareRaw( (unsigned char*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::SHORT )
        r.storeChunk( shareRaw( (short*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::INT )
        r.storeChunk( shareRaw( (int*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::LONG )
       r.storeChunk( shareRaw( (long*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::LONGLONG )
        r.storeChunk( shareRaw( (long long*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::USHORT )
        r.storeChunk( shareRaw( (unsigned short*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::UINT )
        r.storeChunk( shareRaw( (unsigned int*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::ULONG )
        r.storeChunk( shareRaw( (unsigned long*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::ULONGLONG )
        r.storeChunk( shareRaw( (unsigned long long*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::LONG_DOUBLE )
        r.storeChunk( shareRaw( (long double*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::DOUBLE )
        r.storeChunk( shareRaw( (double*)a.mutable_data() ), offset, extent );
    else if( dtype == Datatype::FLOAT )
        r.storeChunk( shareRaw( (float*)a.mutable_data() ), offset, extent );
/* @todo
.value("STRING", Datatype::STRING)
.value("VEC_STRING", Datatype::VEC_STRING)
.value("ARR_DBL_7", Datatype::ARR_DBL_7)
*/
    else if( dtype == Datatype::BOOL )
        r.storeChunk( shareRaw( (bool*)a.mutable_data() ), offset, extent );
    else
        throw std::runtime_error(
            std::string("Datatype '") +
            std::string(py::str(a.dtype())) +
            std::string("' not known in 'storeChunk'!"));
}

/** Store Chunk
 *
 * Called with a py::tuple of slices and a py::array
 */
inline void
store_chunk(RecordComponent & r, py::array & a, py::tuple const & slices)
{
    uint8_t ndim = r.getDimensionality();
    auto const full_extent = r.getExtent();

    Offset offset;
    Extent extent;
    std::vector<bool> flatten;
    std::tie(offset, extent, flatten) = parseTupleSlices(ndim, full_extent, slices);

    store_chunk(r, a, offset, extent, flatten);
}

/** Load Chunk
 *
 * Called with offset and extent that are already in the record component's
 * dimension.
 *
 * Size checks of the requested chunk (spanned data is in valid bounds)
 * will be performed at C++ API part in RecordComponent::loadChunk .
 */
inline py::array
load_chunk(RecordComponent & r, Offset const & offset, Extent const & extent, std::vector<bool> const & flatten)
{
    // some one-size dimensions might be flattended in our output due to selections by index
    size_t const numFlattenDims = std::count(flatten.begin(), flatten.end(), true);
    std::vector< ptrdiff_t > shape(extent.size() - numFlattenDims);
    auto maskIt = flatten.begin();
    std::copy_if(
        std::begin(extent),
        std::end(extent),
        std::begin(shape),
        [&maskIt](std::uint64_t){
            return !*(maskIt++);
        }
    );

    auto const dtype = dtype_to_numpy( r.getDatatype() );

    auto a = py::array( dtype, shape );

    if( r.getDatatype() == Datatype::CHAR )
        r.loadChunk<char>(shareRaw((char*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::UCHAR )
        r.loadChunk<unsigned char>(shareRaw((unsigned char*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::SHORT )
        r.loadChunk<short>(shareRaw((short*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::INT )
        r.loadChunk<int>(shareRaw((int*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::LONG )
        r.loadChunk<long>(shareRaw((long*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::LONGLONG )
        r.loadChunk<long long>(shareRaw((long long*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::USHORT )
        r.loadChunk<unsigned short>(shareRaw((unsigned short*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::UINT )
        r.loadChunk<unsigned int>(shareRaw((unsigned int*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::ULONG )
        r.loadChunk<unsigned long>(shareRaw((unsigned long*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::ULONGLONG )
        r.loadChunk<unsigned long long>(shareRaw((unsigned long long*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::LONG_DOUBLE )
        r.loadChunk<long double>(shareRaw((long double*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::DOUBLE )
        r.loadChunk<double>(shareRaw((double*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::FLOAT )
        r.loadChunk<float>(shareRaw((float*) a.mutable_data()), offset, extent);
    else if( r.getDatatype() == Datatype::BOOL )
        r.loadChunk<bool>(shareRaw((bool*) a.mutable_data()), offset, extent);
    else
        throw std::runtime_error(std::string("Datatype not known in 'loadChunk'!"));

    return a;
}

/** Load Chunk
 *
 * Called with a py::tuple of slices.
 */
inline py::array
load_chunk(RecordComponent & r, py::tuple const & slices)
{
    uint8_t ndim = r.getDimensionality();
    auto const full_extent = r.getExtent();

    Offset offset;
    Extent extent;
    std::vector<bool> flatten;
    std::tie(offset, extent, flatten) = parseTupleSlices(ndim, full_extent, slices);

    return load_chunk(r, offset, extent, flatten);
}

void init_RecordComponent(py::module &m) {
    py::class_<RecordComponent, BaseRecordComponent>(m, "Record_Component")
        .def("__repr__",
            [](RecordComponent const & rc) {
                return "<openPMD.Record_Component of dimensionality '" + std::to_string(rc.getDimensionality()) + "'>";
            }
        )

        .def("set_unit_SI", &RecordComponent::setUnitSI)
        .def("reset_dataset", &RecordComponent::resetDataset)

        .def_property_readonly("ndim", &RecordComponent::getDimensionality)
        .def_property_readonly("shape", &RecordComponent::getExtent)

        // buffer types
        .def("make_constant", [](RecordComponent & rc, py::buffer & a) {
            py::buffer_info buf = a.request();
            auto const dtype = dtype_from_bufferformat( buf.format );

            using DT = Datatype;

            // allow one-element n-dimensional buffers as well
            py::ssize_t numElements = 1;
            if( buf.ndim > 0 ) {
                for( auto d = 0; d < buf.ndim; ++d )
                    numElements *= buf.shape.at(d);
            }

            // Numpy: Handling of arrays and scalars
            // work-around for https://github.com/pybind/pybind11/issues/1224
            // -> passing numpy scalars as buffers needs numpy 1.15+
            //    https://github.com/numpy/numpy/issues/10265
            //    https://github.com/pybind/pybind11/issues/1224#issuecomment-354357392
            // scalars, see PEP 3118
            // requires Numpy 1.15+
            if( numElements == 1 ) {
                // refs:
                //   https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.interface.html
                //   https://docs.python.org/3/library/struct.html#format-characters
                // std::cout << "  scalar type '" << buf.format << "'" << std::endl;
                // typestring: encoding + type + number of bytes
                switch( dtype )
                {
                    case DT::BOOL:
                        return rc.makeConstant( *static_cast<bool*>(buf.ptr) );
                        break;
                    case DT::CHAR:
                        return rc.makeConstant( *static_cast<char*>(buf.ptr) );
                        break;
                    case DT::SHORT:
                        return rc.makeConstant( *static_cast<short*>(buf.ptr) );
                        break;
                    case DT::INT:
                        return rc.makeConstant( *static_cast<int*>(buf.ptr) );
                        break;
                    case DT::LONG:
                        return rc.makeConstant( *static_cast<long*>(buf.ptr) );
                        break;
                    case DT::LONGLONG:
                        return rc.makeConstant( *static_cast<long long*>(buf.ptr) );
                        break;
                    case DT::UCHAR:
                        return rc.makeConstant( *static_cast<unsigned char*>(buf.ptr) );
                        break;
                    case DT::USHORT:
                        return rc.makeConstant( *static_cast<unsigned short*>(buf.ptr) );
                        break;
                    case DT::UINT:
                        return rc.makeConstant( *static_cast<unsigned int*>(buf.ptr) );
                        break;
                    case DT::ULONG:
                        return rc.makeConstant( *static_cast<unsigned long*>(buf.ptr) );
                        break;
                    case DT::ULONGLONG:
                        return rc.makeConstant( *static_cast<unsigned long long*>(buf.ptr) );
                        break;
                    case DT::FLOAT:
                        return rc.makeConstant( *static_cast<float*>(buf.ptr) );
                        break;
                    case DT::DOUBLE:
                        return rc.makeConstant( *static_cast<double*>(buf.ptr) );
                        break;
                    case DT::LONG_DOUBLE:
                        return rc.makeConstant( *static_cast<long double*>(buf.ptr) );
                        break;
                    default:
                        throw std::runtime_error("make_constant: "
                            "Unknown Datatype!");
                }
            }
            else
            {
                throw std::runtime_error("make_constant: "
                    "Only scalar values supported!");
            }

        }, py::arg("value")
        )
        // allowed python intrinsics, after (!) buffer matching
        .def("make_constant", &RecordComponent::makeConstant<char>,
            py::arg("value"))
        .def("make_constant", &RecordComponent::makeConstant<long>,
            py::arg("value"))
        .def("make_constant", &RecordComponent::makeConstant<double>,
            py::arg("value"))
        .def("make_constant", &RecordComponent::makeConstant<bool>,
            py::arg("value"))

        // TODO if we also want to support scalar arrays, we have to switch
        //      py::array for py::buffer as in Attributable
        //      https://github.com/pybind/pybind11/pull/1537

        // slicing protocol
        .def("__getitem__", [](RecordComponent & r, py::tuple const & slices) {
            return load_chunk(r, slices);
        },
            py::arg("tuple of index slices")
        )
        .def("__getitem__", [](RecordComponent & r, py::slice const & slice_obj) {
            auto const slices = py::make_tuple(slice_obj);
            return load_chunk(r, slices);
        },
            py::arg("slice")
        )
        .def("__getitem__", [](RecordComponent & r, py::int_ const & slice_obj) {
            auto const slices = py::make_tuple(slice_obj);
            return load_chunk(r, slices);
        },
            py::arg("axis index")
        )

        .def("__setitem__", [](RecordComponent & r, py::tuple const & slices, py::array & a ) {
            store_chunk(r, a, slices);
        },
            py::arg("tuple of index slices"),
            py::arg("array with values to assign")
        )
        .def("__setitem__", [](RecordComponent & r, py::slice const & slice_obj, py::array & a ) {
            auto const slices = py::make_tuple(slice_obj);
            store_chunk(r, a, slices);
        },
            py::arg("slice"),
            py::arg("array with values to assign")
        )
        .def("__setitem__", [](RecordComponent & r, py::int_ const & slice_obj, py::array & a ) {
            auto const slices = py::make_tuple(slice_obj);
            store_chunk(r, a, slices);
        },
            py::arg("axis index"),
            py::arg("array with values to assign")
        )

        // deprecated: pass-through C++ API
        .def("load_chunk", [](RecordComponent & r, Offset const & offset_in, Extent const & extent_in) {
            uint8_t ndim = r.getDimensionality();

            // default arguments
            //   offset = {0u}: expand to right dim {0u, 0u, ...}
            Offset offset = offset_in;
            if( offset_in.size() == 1u && offset_in.at(0) == 0u )
                offset = Offset(ndim, 0u);

            //   extent = {-1u}: take full size
            Extent extent(ndim, 1u);
            if( extent_in.size() == 1u && extent_in.at(0) == -1u )
            {
                extent = r.getExtent();
                for( uint8_t i = 0u; i < ndim; ++i )
                    extent[i] -= offset[i];
            }
            else
                extent = extent_in;

            std::vector<bool> flatten(ndim, false);
            return load_chunk(r, offset, extent, flatten);
        },
            py::arg_v("offset", Offset(1,  0u), "np.zeros(Record_Component.shape)"),
            py::arg_v("extent", Extent(1, -1u), "Record_Component.shape")
        )

        // deprecated: pass-through C++ API
        .def("store_chunk", [](RecordComponent & r, py::array & a, Offset const & offset_in, Extent const & extent_in) {
            // default arguments
            //   offset = {0u}: expand to right dim {0u, 0u, ...}
            Offset offset = offset_in;
            if( offset_in.size() == 1u && offset_in.at(0) == 0u && a.ndim() > 1u )
                offset = Offset(a.ndim(), 0u);

            //   extent = {-1u}: take full size
            Extent extent(a.ndim(), 1u);
            if( extent_in.size() == 1u && extent_in.at(0) == -1u )
                for( auto d = 0; d < a.ndim(); ++d )
                    extent.at(d) = a.shape()[d];
            else
                extent = extent_in;

            std::vector<bool> flatten(r.getDimensionality(), false);
            store_chunk(r, a, offset, extent, flatten);
        },
            py::arg("array"),
            py::arg_v("offset", Offset(1,  0u), "np.zeros_like(array)"),
            py::arg_v("extent", Extent(1, -1u), "array.shape")
        )

        .def_property_readonly_static("SCALAR", [](py::object){ return RecordComponent::SCALAR; })
    ;

    py::enum_<RecordComponent::Allocation>(m, "Allocation")
        .value("USER", RecordComponent::Allocation::USER)
        .value("API", RecordComponent::Allocation::API)
        .value("AUTO", RecordComponent::Allocation::AUTO)
    ;
}
