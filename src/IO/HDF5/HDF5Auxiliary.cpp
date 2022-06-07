/* Copyright 2017-2021 Fabian Koller, Felix Schmitt, Axel Huebl
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
#include "openPMD/config.hpp"
#if openPMD_HAVE_HDF5
#include "openPMD/IO/HDF5/HDF5Auxiliary.hpp"
#include "openPMD/IO/HDF5/HDF5FilePosition.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"

#include <hdf5.h>

#include <array>
#include <complex>
#include <map>
#include <stack>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

#if openPMD_USE_VERIFY
#define VERIFY(CONDITION, TEXT)                                                \
    {                                                                          \
        if (!(CONDITION))                                                      \
            throw std::runtime_error((TEXT));                                  \
    }
#else
#define VERIFY(CONDITION, TEXT)                                                \
    do                                                                         \
    {                                                                          \
        (void)sizeof(CONDITION);                                               \
    } while (0)
#endif

hid_t openPMD::GetH5DataType::operator()(Attribute const &att)
{
    using DT = Datatype;
    switch (att.dtype)
    {
    case DT::CHAR:
    case DT::VEC_CHAR:
        return H5Tcopy(H5T_NATIVE_CHAR);
    case DT::UCHAR:
    case DT::VEC_UCHAR:
        return H5Tcopy(H5T_NATIVE_UCHAR);
    case DT::SHORT:
    case DT::VEC_SHORT:
        return H5Tcopy(H5T_NATIVE_SHORT);
    case DT::INT:
    case DT::VEC_INT:
        return H5Tcopy(H5T_NATIVE_INT);
    case DT::LONG:
    case DT::VEC_LONG:
        return H5Tcopy(H5T_NATIVE_LONG);
    case DT::LONGLONG:
    case DT::VEC_LONGLONG:
        return H5Tcopy(H5T_NATIVE_LLONG);
    case DT::USHORT:
    case DT::VEC_USHORT:
        return H5Tcopy(H5T_NATIVE_USHORT);
    case DT::UINT:
    case DT::VEC_UINT:
        return H5Tcopy(H5T_NATIVE_UINT);
    case DT::ULONG:
    case DT::VEC_ULONG:
        return H5Tcopy(H5T_NATIVE_ULONG);
    case DT::ULONGLONG:
    case DT::VEC_ULONGLONG:
        return H5Tcopy(H5T_NATIVE_ULLONG);
    case DT::FLOAT:
    case DT::VEC_FLOAT:
        return H5Tcopy(H5T_NATIVE_FLOAT);
    case DT::DOUBLE:
    case DT::ARR_DBL_7:
    case DT::VEC_DOUBLE:
        return H5Tcopy(H5T_NATIVE_DOUBLE);
    case DT::LONG_DOUBLE:
    case DT::VEC_LONG_DOUBLE:
        return H5Tcopy(H5T_NATIVE_LDOUBLE);
    case DT::CFLOAT:
    case DT::VEC_CFLOAT:
        return H5Tcopy(m_userTypes.at(typeid(std::complex<float>).name()));
    case DT::CDOUBLE:
    case DT::VEC_CDOUBLE:
        return H5Tcopy(m_userTypes.at(typeid(std::complex<double>).name()));
    case DT::CLONG_DOUBLE:
    case DT::VEC_CLONG_DOUBLE:
        return H5Tcopy(
            m_userTypes.at(typeid(std::complex<long double>).name()));
    case DT::STRING: {
        hid_t string_t_id = H5Tcopy(H5T_C_S1);
        size_t const max_len = att.get<std::string>().size();
        VERIFY(max_len > 0, "[HDF5] max_len must be >0 for STRING");
        herr_t status = H5Tset_size(string_t_id, max_len);
        VERIFY(
            status >= 0,
            "[HDF5] Internal error: Failed in H5Tset_size for STRING");
        return string_t_id;
    }
    case DT::VEC_STRING: {
        hid_t string_t_id = H5Tcopy(H5T_C_S1);
        size_t max_len = 0;
        for (std::string const &s : att.get<std::vector<std::string> >())
            max_len = std::max(max_len, s.size());
        VERIFY(max_len > 0, "[HDF5] max_len must be >0 for VEC_STRING");
        herr_t status = H5Tset_size(string_t_id, max_len);
        VERIFY(
            status >= 0,
            "[HDF5] Internal error: Failed in H5Tset_size for VEC_STRING");
        return string_t_id;
    }
    case DT::BOOL:
        return H5Tcopy(m_userTypes.at(typeid(bool).name()));
    case DT::DATATYPE:
        throw std::runtime_error("[HDF5] Meta-Datatype leaked into IO");
    case DT::UNDEFINED:
        throw std::runtime_error(
            "[HDF5] Unknown Attribute datatype (HDF5 datatype)");
    default:
        throw std::runtime_error("[HDF5] Datatype not implemented");
    }
}

hid_t openPMD::getH5DataSpace(Attribute const &att)
{
    using DT = Datatype;
    switch (att.dtype)
    {
    case DT::CHAR:
    case DT::UCHAR:
    case DT::SHORT:
    case DT::INT:
    case DT::LONG:
    case DT::LONGLONG:
    case DT::USHORT:
    case DT::UINT:
    case DT::ULONG:
    case DT::ULONGLONG:
    case DT::FLOAT:
    case DT::DOUBLE:
    case DT::LONG_DOUBLE:
    case DT::CFLOAT:
    case DT::CDOUBLE:
    case DT::CLONG_DOUBLE:
    case DT::STRING:
    case DT::BOOL:
        return H5Screate(H5S_SCALAR);
    case DT::VEC_CHAR: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<char> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_SHORT: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<short> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_INT: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<int> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_LONG: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<long> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_LONGLONG: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<long long> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_UCHAR: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<unsigned char> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_USHORT: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<unsigned short> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_UINT: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<unsigned int> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_ULONG: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<unsigned long> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_ULONGLONG: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<unsigned long long> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_FLOAT: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<float> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_DOUBLE: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<double> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_LONG_DOUBLE: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<long double> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_CFLOAT: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {
            att.get<std::vector<std::complex<float> > >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_CDOUBLE: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {
            att.get<std::vector<std::complex<double> > >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_CLONG_DOUBLE: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {
            att.get<std::vector<std::complex<long double> > >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::VEC_STRING: {
        hid_t vec_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {att.get<std::vector<std::string> >().size()};
        H5Sset_extent_simple(vec_t_id, 1, dims, nullptr);
        return vec_t_id;
    }
    case DT::ARR_DBL_7: {
        hid_t array_t_id = H5Screate(H5S_SIMPLE);
        hsize_t dims[1] = {7};
        H5Sset_extent_simple(array_t_id, 1, dims, nullptr);
        return array_t_id;
    }
    case DT::UNDEFINED:
        throw std::runtime_error("Unknown Attribute datatype (HDF5 dataspace)");
    default:
        throw std::runtime_error("Datatype not implemented in HDF5 IO");
    }
}

std::string openPMD::concrete_h5_file_position(Writable *w)
{
    std::stack<Writable *> hierarchy;
    if (!w->abstractFilePosition)
        w = w->parent;
    while (w)
    {
        hierarchy.push(w);
        w = w->parent;
    }

    std::string pos;
    while (!hierarchy.empty())
    {
        pos += std::dynamic_pointer_cast<HDF5FilePosition>(
                   hierarchy.top()->abstractFilePosition)
                   ->location;
        hierarchy.pop();
    }

    return auxiliary::replace_all(pos, "//", "/");
}

std::vector<hsize_t> openPMD::getOptimalChunkDims(
    std::vector<hsize_t> const dims, size_t const typeSize)
{
    auto const ndims = dims.size();
    std::vector<hsize_t> chunk_dims(dims.size());

    // chunk sizes in KiByte
    constexpr std::array<size_t, 7u> CHUNK_SIZES_KiB{
        {4096u, 2048u, 1024u, 512u, 256u, 128u, 64u}};

    size_t total_data_size = typeSize;
    size_t max_chunk_size = typeSize;
    size_t target_chunk_size = 0u;

    // compute the order of dimensions (descending)
    // large dataset dimensions should have larger chunk sizes
    std::multimap<hsize_t, uint32_t> dims_order;
    for (uint32_t i = 0; i < ndims; ++i)
        dims_order.insert(std::make_pair(dims[i], i));

    for (uint32_t i = 0; i < ndims; ++i)
    {
        // initial number of chunks per dimension
        chunk_dims[i] = 1;

        // try to make at least two chunks for each dimension
        size_t half_dim = dims[i] / 2;

        // compute sizes
        max_chunk_size *= (half_dim > 0) ? half_dim : 1;
        total_data_size *= dims[i];
    }

    // compute the target chunk size
    for (auto const &chunk_size : CHUNK_SIZES_KiB)
    {
        target_chunk_size = chunk_size * 1024;
        if (target_chunk_size <= max_chunk_size)
            break;
    }

    size_t current_chunk_size = typeSize;
    size_t last_chunk_diff = target_chunk_size;
    std::multimap<hsize_t, uint32_t>::const_iterator current_index =
        dims_order.begin();

    while (current_chunk_size < target_chunk_size)
    {
        // test if increasing chunk size optimizes towards target chunk size
        size_t chunk_diff = target_chunk_size - (current_chunk_size * 2u);
        if (chunk_diff >= last_chunk_diff)
            break;

        // find next dimension to increase chunk size for
        int can_increase_dim = 0;
        for (uint32_t d = 0; d < ndims; ++d)
        {
            int current_dim = current_index->second;

            // increasing chunk size possible
            if (chunk_dims[current_dim] * 2 <= dims[current_dim])
            {
                chunk_dims[current_dim] *= 2;
                current_chunk_size *= 2;
                can_increase_dim = 1;
            }

            current_index++;
            if (current_index == dims_order.end())
                current_index = dims_order.begin();

            if (can_increase_dim)
                break;
        }

        // can not increase chunk size in any dimension
        // we must use the current chunk sizes
        if (!can_increase_dim)
            break;

        last_chunk_diff = chunk_diff;
    }

    return chunk_dims;
}

#endif
