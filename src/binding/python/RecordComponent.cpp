/* Copyright 2018 Axel Huebl
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

#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/auxiliary/ShareRaw.hpp"

#include <string>
#include <algorithm>
#include <iostream>
#include <cstdint>

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

        .def("make_constant", &RecordComponent::makeConstant<float>)
        .def("make_constant", &RecordComponent::makeConstant<double>)
        .def("make_constant", &RecordComponent::makeConstant<long double>)
        .def("make_constant", &RecordComponent::makeConstant<int16_t>)
        .def("make_constant", &RecordComponent::makeConstant<int32_t>)
        .def("make_constant", &RecordComponent::makeConstant<int64_t>)
        .def("make_constant", &RecordComponent::makeConstant<uint16_t>)
        .def("make_constant", &RecordComponent::makeConstant<uint32_t>)
        .def("make_constant", &RecordComponent::makeConstant<uint64_t>)
        .def("make_constant", &RecordComponent::makeConstant<char>)
        .def("make_constant", &RecordComponent::makeConstant<unsigned char>)
        .def("make_constant", &RecordComponent::makeConstant<bool>)

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
            E_x.loadChunk(chunk_offset, chunk_extent, chunk_data);

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
        .def("load_chunk", [](RecordComponent & r, Offset const & offset, Extent const & extent) {
            std::vector<ptrdiff_t> c(extent.size());
            std::copy(std::begin(extent), std::end(extent), std::begin(c));
            return py::array( c, r.loadChunk<double>(offset, extent).get() );
        })

        // deprecated: pass-through C++ API
        .def("store_chunk", [](RecordComponent & r, Offset const & offset, Extent const & extent, py::array & a) {
            // cast py::array to proper Datatype
            // auto buf = py::array::ensure( py::dtype("double") ) ; //r.getDatatype() );
            //if( !buf ) return false;

            // @todo verify offset + extend fit in dataset extend
            // @todo verify array is contigous
            // @todo enforce or transform C/F order

            // @todo keep locked until flush() is performed
            // a.flags.writable = false;
            // a.flags.owndata = false;
            py::print( py::str(a.dtype()) );
            // py::print( py::str(buf.dtype()) );

            if( a.dtype().is(py::dtype("b")) )
                r.storeChunk( offset, extent, shareRaw( (char*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("B")) )
                r.storeChunk( offset, extent, shareRaw( (unsigned char*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("int16")) )
                r.storeChunk( offset, extent, shareRaw( (int16_t*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("int32")) )
                r.storeChunk( offset, extent, shareRaw( (int32_t*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("int64")) )
                r.storeChunk( offset, extent, shareRaw( (int64_t*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("uint16")) )
                r.storeChunk( offset, extent, shareRaw( (uint16_t*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("uint32")) )
                r.storeChunk( offset, extent, shareRaw( (uint32_t*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("uint64")) )
                r.storeChunk( offset, extent, shareRaw( (uint64_t*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("longdouble")) )
                r.storeChunk( offset, extent, shareRaw( (long double*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("double")) )
                r.storeChunk( offset, extent, shareRaw( (double*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("float")) )
                r.storeChunk( offset, extent, shareRaw( (float*)a.mutable_data() ) );
/* @todo
        .value("STRING", Datatype::STRING)
        .value("VEC_CHAR", Datatype::VEC_CHAR)
        .value("VEC_INT16", Datatype::VEC_INT16)
        .value("VEC_INT32", Datatype::VEC_INT32)
        .value("VEC_INT64", Datatype::VEC_INT64)
        .value("VEC_UCHAR", Datatype::VEC_UCHAR)
        .value("VEC_UINT16", Datatype::VEC_UINT16)
        .value("VEC_UINT32", Datatype::VEC_UINT32)
        .value("VEC_UINT64", Datatype::VEC_UINT64)
        .value("VEC_FLOAT", Datatype::VEC_FLOAT)
        .value("VEC_DOUBLE", Datatype::VEC_DOUBLE)
        .value("VEC_LONG_DOUBLE", Datatype::VEC_LONG_DOUBLE)
        .value("VEC_STRING", Datatype::VEC_STRING)
        .value("ARR_DBL_7", Datatype::ARR_DBL_7)
*/
            else if( a.dtype().is(py::dtype("bool")) )
                r.storeChunk( offset, extent, shareRaw( (bool*)a.mutable_data() ) );
            else
                throw std::runtime_error(std::string("Datatype '") + std::string(py::str(a.dtype())) + std::string("' not known in 'store_chunk'!"));
        })

        .def_property_readonly_static("SCALAR", [](py::object){ return RecordComponent::SCALAR; })
    ;

    py::enum_<RecordComponent::Allocation>(m, "Allocation")
        .value("USER", RecordComponent::Allocation::USER)
        .value("API", RecordComponent::Allocation::API)
        .value("AUTO", RecordComponent::Allocation::AUTO)
    ;
}
