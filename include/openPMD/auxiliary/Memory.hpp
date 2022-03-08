/* Copyright 2017-2021 Fabian Koller
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
#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/Datatype.hpp"

#include <complex>
#include <functional>
#include <memory>
#include <utility>

namespace openPMD
{
namespace auxiliary
{
    inline std::unique_ptr<void, std::function<void(void *)> >
    allocatePtr(Datatype dtype, uint64_t numPoints)
    {
        void *data = nullptr;
        std::function<void(void *)> del = [](void *) {};
        switch (dtype)
        {
            using DT = Datatype;
        case DT::VEC_STRING:
            data = new char *[numPoints];
            del = [](void *p) { delete[] static_cast<char **>(p); };
            break;
        case DT::VEC_LONG_DOUBLE:
        case DT::LONG_DOUBLE:
            data = new long double[numPoints];
            del = [](void *p) { delete[] static_cast<long double *>(p); };
            break;
        case DT::ARR_DBL_7:
        case DT::VEC_DOUBLE:
        case DT::DOUBLE:
            data = new double[numPoints];
            del = [](void *p) { delete[] static_cast<double *>(p); };
            break;
        case DT::VEC_FLOAT:
        case DT::FLOAT:
            data = new float[numPoints];
            del = [](void *p) { delete[] static_cast<float *>(p); };
            break;
        case DT::VEC_CLONG_DOUBLE:
        case DT::CLONG_DOUBLE:
            data = new std::complex<long double>[numPoints];
            del = [](void *p) {
                delete[] static_cast<std::complex<long double> *>(p);
            };
            break;
        case DT::VEC_CDOUBLE:
        case DT::CDOUBLE:
            data = new std::complex<double>[numPoints];
            del = [](void *p) {
                delete[] static_cast<std::complex<double> *>(p);
            };
            break;
        case DT::VEC_CFLOAT:
        case DT::CFLOAT:
            data = new std::complex<float>[numPoints];
            del = [](void *p) {
                delete[] static_cast<std::complex<float> *>(p);
            };
            break;
        case DT::VEC_SHORT:
        case DT::SHORT:
            data = new short[numPoints];
            del = [](void *p) { delete[] static_cast<short *>(p); };
            break;
        case DT::VEC_INT:
        case DT::INT:
            data = new int[numPoints];
            del = [](void *p) { delete[] static_cast<int *>(p); };
            break;
        case DT::VEC_LONG:
        case DT::LONG:
            data = new long[numPoints];
            del = [](void *p) { delete[] static_cast<long *>(p); };
            break;
        case DT::VEC_LONGLONG:
        case DT::LONGLONG:
            data = new long long[numPoints];
            del = [](void *p) { delete[] static_cast<long long *>(p); };
            break;
        case DT::VEC_USHORT:
        case DT::USHORT:
            data = new unsigned short[numPoints];
            del = [](void *p) { delete[] static_cast<unsigned short *>(p); };
            break;
        case DT::VEC_UINT:
        case DT::UINT:
            data = new unsigned int[numPoints];
            del = [](void *p) { delete[] static_cast<unsigned int *>(p); };
            break;
        case DT::VEC_ULONG:
        case DT::ULONG:
            data = new unsigned long[numPoints];
            del = [](void *p) { delete[] static_cast<unsigned long *>(p); };
            break;
        case DT::VEC_ULONGLONG:
        case DT::ULONGLONG:
            data = new unsigned long long[numPoints];
            del = [](void *p) {
                delete[] static_cast<unsigned long long *>(p);
            };
            break;
        case DT::VEC_CHAR:
        case DT::CHAR:
            data = new char[numPoints];
            del = [](void *p) { delete[] static_cast<char *>(p); };
            break;
        case DT::VEC_UCHAR:
        case DT::UCHAR:
            data = new unsigned char[numPoints];
            del = [](void *p) { delete[] static_cast<unsigned char *>(p); };
            break;
        case DT::BOOL:
            data = new bool[numPoints];
            del = [](void *p) { delete[] static_cast<bool *>(p); };
            break;
        case DT::STRING:
            /* user assigns c_str pointer */
            break;
        case DT::UNDEFINED:
        default:
            throw std::runtime_error(
                "Unknown Attribute datatype (Pointer allocation)");
        }

        return std::unique_ptr<void, std::function<void(void *)> >(data, del);
    }

    inline std::unique_ptr<void, std::function<void(void *)> >
    allocatePtr(Datatype dtype, Extent const &e)
    {
        uint64_t numPoints = 1u;
        for (auto const &dimensionSize : e)
            numPoints *= dimensionSize;
        return allocatePtr(dtype, numPoints);
    }

} // namespace auxiliary
} // namespace openPMD
