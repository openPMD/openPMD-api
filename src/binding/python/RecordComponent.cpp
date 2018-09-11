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
#include <pybind11/stl.h>

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
        .def("make_constant", &RecordComponent::makeConstant<short>)
        .def("make_constant", &RecordComponent::makeConstant<int>)
        .def("make_constant", &RecordComponent::makeConstant<long>)
        .def("make_constant", &RecordComponent::makeConstant<long long>)
        .def("make_constant", &RecordComponent::makeConstant<unsigned short>)
        .def("make_constant", &RecordComponent::makeConstant<unsigned int>)
        .def("make_constant", &RecordComponent::makeConstant<unsigned long>)
        .def("make_constant", &RecordComponent::makeConstant<unsigned long long>)
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
            std::vector<ptrdiff_t> shape(extent.size());
            std::copy(std::begin(extent), std::end(extent), std::begin(shape));

            auto dtype = py::dtype("void");

            if( r.getDatatype() == Datatype::CHAR ) dtype = py::dtype("b");
            else if( r.getDatatype() == Datatype::UCHAR ) dtype = py::dtype("B");
            else if( r.getDatatype() == Datatype::SHORT ) dtype = py::dtype("short");
            else if( r.getDatatype() == Datatype::INT ) dtype = py::dtype("intc");
            else if( r.getDatatype() == Datatype::LONG ) dtype = py::dtype("int_");
            else if( r.getDatatype() == Datatype::LONGLONG ) dtype = py::dtype("longlong");
            else if( r.getDatatype() == Datatype::USHORT ) dtype = py::dtype("ushort");
            else if( r.getDatatype() == Datatype::UINT ) dtype = py::dtype("uintc");
            else if( r.getDatatype() == Datatype::ULONG ) dtype = py::dtype("uint");
            else if( r.getDatatype() == Datatype::ULONGLONG ) dtype = py::dtype("ulonglong");
            else if( r.getDatatype() == Datatype::LONG_DOUBLE ) dtype = py::dtype("longdouble");
            else if( r.getDatatype() == Datatype::DOUBLE ) dtype = py::dtype("double");
            else if( r.getDatatype() == Datatype::FLOAT ) dtype = py::dtype("float");

            /* @todo
            .value("STRING", Datatype::STRING)
            .value("VEC_CHAR", Datatype::VEC_CHAR)
            .value("VEC_SHORT", Datatype::VEC_SHORT)
            .value("VEC_INT", Datatype::VEC_INT)
            .value("VEC_LONG", Datatype::VEC_LONG)
            .value("VEC_LONGLONG", Datatype::VEC_LONGLONG)
            .value("VEC_UCHAR", Datatype::VEC_UCHAR)
            .value("VEC_USHORT", Datatype::VEC_USHORT)
            .value("VEC_UINT", Datatype::VEC_UINT)
            .value("VEC_ULONG", Datatype::VEC_ULONG)
            .value("VEC_ULONGLONG", Datatype::VEC_ULONGLONG)
            .value("VEC_FLOAT", Datatype::VEC_FLOAT)
            .value("VEC_DOUBLE", Datatype::VEC_DOUBLE)
            .value("VEC_LONG_DOUBLE", Datatype::VEC_LONG_DOUBLE)
            .value("VEC_STRING", Datatype::VEC_STRING)
            .value("ARR_DBL_7", Datatype::ARR_DBL_7)
            */

            else if( r.getDatatype() == Datatype::BOOL ) dtype = py::dtype("bool");
            else
                throw std::runtime_error(std::string("Datatype not known in 'load_chunk'!"));

            auto a = py::array( dtype, shape );

            if( r.getDatatype() == Datatype::CHAR )
                r.loadChunk<char>(offset, extent, shareRaw((char*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::UCHAR )
                r.loadChunk<unsigned char>(offset, extent, shareRaw((unsigned char*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::SHORT )
                r.loadChunk<short>(offset, extent, shareRaw((short*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::INT )
                r.loadChunk<int>(offset, extent, shareRaw((int*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::LONG )
                r.loadChunk<long>(offset, extent, shareRaw((long*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::LONGLONG )
                r.loadChunk<long long>(offset, extent, shareRaw((long long*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::USHORT )
                r.loadChunk<unsigned short>(offset, extent, shareRaw((unsigned short*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::UINT )
                r.loadChunk<unsigned int>(offset, extent, shareRaw((unsigned int*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::ULONG )
                r.loadChunk<unsigned long>(offset, extent, shareRaw((unsigned long*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::ULONGLONG )
                r.loadChunk<unsigned long long>(offset, extent, shareRaw((unsigned long long*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::LONG_DOUBLE )
                r.loadChunk<long double>(offset, extent, shareRaw((long double*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::DOUBLE )
                r.loadChunk<double>(offset, extent, shareRaw((double*) a.mutable_data()));
            else if( r.getDatatype() == Datatype::FLOAT )
                r.loadChunk<float>(offset, extent, shareRaw((float*) a.mutable_data()));

            /* @todo
            .value("STRING", Datatype::STRING)
            .value("VEC_CHAR", Datatype::VEC_CHAR)
            .value("VEC_SHORT", Datatype::VEC_SHORT)
            .value("VEC_INT", Datatype::VEC_INT)
            .value("VEC_LONG", Datatype::VEC_LONG)
            .value("VEC_LONGLONG", Datatype::VEC_LONGLONG)
            .value("VEC_UCHAR", Datatype::VEC_UCHAR)
            .value("VEC_USHORT", Datatype::VEC_USHORT)
            .value("VEC_UINT", Datatype::VEC_UINT)
            .value("VEC_ULONG", Datatype::VEC_ULONG)
            .value("VEC_ULONGLONG", Datatype::VEC_ULONGLONG)
            .value("VEC_FLOAT", Datatype::VEC_FLOAT)
            .value("VEC_DOUBLE", Datatype::VEC_DOUBLE)
            .value("VEC_LONG_DOUBLE", Datatype::VEC_LONG_DOUBLE)
            .value("VEC_STRING", Datatype::VEC_STRING)
            .value("ARR_DBL_7", Datatype::ARR_DBL_7)
            */

            else if( r.getDatatype() == Datatype::BOOL )
                r.loadChunk<bool>(offset, extent, shareRaw((bool*) a.mutable_data()));
            else
                throw std::runtime_error(std::string("Datatype not known in 'load_chunk'!"));

            return a;
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
            // py::print( py::str(a.dtype()) );
            // py::print( py::str(buf.dtype()) );

            // ref: https://docs.scipy.org/doc/numpy/user/basics.types.html
            // ref: https://github.com/numpy/numpy/issues/10678#issuecomment-369363551
            if( a.dtype().is(py::dtype("b")) )
                r.storeChunk( offset, extent, shareRaw( (char*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("B")) )
                r.storeChunk( offset, extent, shareRaw( (unsigned char*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("short")) )
                r.storeChunk( offset, extent, shareRaw( (short*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("intc")) )
                r.storeChunk( offset, extent, shareRaw( (int*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("int_")) )
               r.storeChunk( offset, extent, shareRaw( (long*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("longlong")) )
                r.storeChunk( offset, extent, shareRaw( (long long*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("ushort")) )
                r.storeChunk( offset, extent, shareRaw( (unsigned short*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("uintc")) )
                r.storeChunk( offset, extent, shareRaw( (unsigned int*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("uint")) )
                r.storeChunk( offset, extent, shareRaw( (unsigned long*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("ulonglong")) )
                r.storeChunk( offset, extent, shareRaw( (unsigned long long*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("longdouble")) )
                r.storeChunk( offset, extent, shareRaw( (long double*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("double")) )
                r.storeChunk( offset, extent, shareRaw( (double*)a.mutable_data() ) );
            else if( a.dtype().is(py::dtype("single")) ) // note: np.float is an alias for float64
                r.storeChunk( offset, extent, shareRaw( (float*)a.mutable_data() ) );
/* @todo
        .value("STRING", Datatype::STRING)
        .value("VEC_CHAR", Datatype::VEC_CHAR)
        .value("VEC_SHORT", Datatype::VEC_SHORT)
        .value("VEC_INT", Datatype::VEC_INT)
        .value("VEC_LONG", Datatype::VEC_LONG)
        .value("VEC_LONGLONG", Datatype::VEC_LONGLONG)
        .value("VEC_UCHAR", Datatype::VEC_UCHAR)
        .value("VEC_USHORT", Datatype::VEC_USHORT)
        .value("VEC_UINT", Datatype::VEC_UINT)
        .value("VEC_ULONG", Datatype::VEC_ULONG)
        .value("VEC_ULONGLONG", Datatype::VEC_ULONGLONG)
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
