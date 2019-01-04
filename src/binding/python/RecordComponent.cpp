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
#include <iostream>
#include <cstdint>
#include <exception>

namespace py = pybind11;
using namespace openPMD;


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

        /* draft of the slicing protocol
        .def("__getitem__", [](RecordComponent const & r, py::slice s) {
            size_t start, stop, step, slicelength;
            if( !slice.compute( r.getExtent().size(), &start, &stop, &step, &slicelength ) )
                throw py::error_already_set();

            py::array a;
            a.resize( s.getExtent() );
            std::fill( a.mutable_data(), a.mutable_data() + a.size(), 0. );

            // @todo keep locked until flush() is performed
            // a.flags.writable = false;
            // a.flags.owndata = false;

            Offset chunk_offset = {1, 1, 1};
            Extent chunk_extent = {2, 2, 1};
            std::unique_ptr< double[] > chunk_data;
            E_x.loadChunk(chunk_data, chunk_offset, chunk_extent);

            return a;
        })
        .def("__setitem__", [](RecordComponent const & r, py::slice s, py::array & a ) {
            size_t start, stop, step, slicelength;
            if( !slice.compute( r.getExtent().size(), &start, &stop, &step, &slicelength ) )
                throw py::error_already_set();
            if( slicelength != value.size() )
                throw std::runtime_error("Left and right hand size of assignment have different sizes!");

            Extent extent;
            Offset offset;
            for( size_t i = 0; i < slicelength; ++i ) {
                std::cout << start << " " << stop << " " << step << " " << slicelength << std::endl;
                offset[i] = start;
                extent[i] = stop - start;
                start += step;
            }

            // @todo keep locked until flush() is performed
            // a.flags.writable = false;
            // a.flags.owndata = false;

            // r.storeChunk();
        }) */

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

            std::vector<ptrdiff_t> shape(extent.size());
            std::copy(std::begin(extent), std::end(extent), std::begin(shape));

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
                throw std::runtime_error(std::string("Datatype not known in 'load_chunk'!"));

            return a;
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

            for( auto d = 0; d < a.ndim(); ++d )
            {
                // std::cout << "    stride '" << d << "': "
                //           << a.strides()[d] / a.itemsize()
                //           << " - " << a.shape()[d] << std::endl;
                if( a.strides()[d] != a.shape()[d] * a.itemsize() ) // general criteria
                {
                    if( a.ndim() == 1u && a.strides()[0] > a.shape()[0] * a.itemsize() )
                        ; // ok in 1D
                    else if( a.strides()[d] == a.itemsize() )
                        ; // ok to stride on an element level
                    else
                        throw std::runtime_error("store_chunk: "
                            "stride handling not implemented!");
                }
            }
            // @todo in order to implement stride handling, one needs to
            //       loop over the input data strides during write below

            // @todo verify offset + extend fit in dataset extend
            // @todo keep locked until flush() is performed
            // a.flags.writable = false;
            // a.flags.owndata = false;

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
                throw std::runtime_error(std::string("Datatype '") + std::string(py::str(a.dtype())) + std::string("' not known in 'store_chunk'!"));
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
