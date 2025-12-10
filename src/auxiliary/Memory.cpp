/* Copyright 2017-2025 Fabian Koller, Franz Poeschel
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

#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/ChunkInfo.hpp"
#include "openPMD/auxiliary/Memory_internal.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

#include <any>
#include <complex>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace openPMD::auxiliary
{
std::unique_ptr<void, std::function<void(void *)>>
allocatePtr(Datatype dtype, uint64_t numPoints)
{
    void *data = nullptr;
    std::function<void(void *)> del = [](void *) {};
    switch (dtype)
    {
        using DT = Datatype;
    case DT::VEC_STRING:
        // NOLINTNEXTLINE(bugprone-multi-level-implicit-pointer-conversion)
        data = static_cast<void *>(new char *[numPoints]);
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
        del = [](void *p) { delete[] static_cast<std::complex<double> *>(p); };
        break;
    case DT::VEC_CFLOAT:
    case DT::CFLOAT:
        data = new std::complex<float>[numPoints];
        del = [](void *p) { delete[] static_cast<std::complex<float> *>(p); };
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
        del = [](void *p) { delete[] static_cast<unsigned long long *>(p); };
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
    case DT::VEC_SCHAR:
    case DT::SCHAR:
        data = new signed char[numPoints];
        del = [](void *p) { delete[] static_cast<signed char *>(p); };
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

    return std::unique_ptr<void, std::function<void(void *)>>(data, del);
}

std::unique_ptr<void, std::function<void(void *)>>
allocatePtr(Datatype dtype, Extent const &e)
{
    uint64_t numPoints = 1u;
    for (auto const &dimensionSize : e)
        numPoints *= dimensionSize;
    return allocatePtr(dtype, numPoints);
}

WriteBuffer::CopyableUniquePtr::CopyableUniquePtr() = default;

WriteBuffer::CopyableUniquePtr::CopyableUniquePtr(
    UniquePtrWithLambda<void> ptr_in)
    : parent_t{std::make_shared<UniquePtrWithLambda<void>>(std::move(ptr_in))}
{}

auto WriteBuffer::CopyableUniquePtr::get() -> void *
{
    return (**this).get();
}

auto WriteBuffer::CopyableUniquePtr::get() const -> void const *
{
    return (**this).get();
}

auto WriteBuffer::CopyableUniquePtr::release() -> UniquePtrWithLambda<void>
{
    if (parent_t::use_count() > 1)
    {
        throw error::Internal(
            "Control flow error: UniquePtr variant of WriteBuffer "
            "has been copied.");
    }
    UniquePtrWithLambda<void> res = std::move(**this);
    this->reset();
    return res;
}

WriteBuffer::WriteBuffer() : m_buffer(std::make_any<CopyableUniquePtr>())
{}
WriteBuffer::WriteBuffer(std::shared_ptr<void const> ptr)
    : m_buffer(std::make_any<WriteBufferTypes>(std::move(ptr)))
{}
WriteBuffer::WriteBuffer(UniquePtrWithLambda<void> ptr)
    : m_buffer(
          std::make_any<WriteBufferTypes>(CopyableUniquePtr(std::move(ptr))))
{}

WriteBuffer::WriteBuffer(WriteBuffer &&) noexcept = default;
WriteBuffer &WriteBuffer::operator=(WriteBuffer &&) noexcept = default;

WriteBuffer const &WriteBuffer::operator=(std::shared_ptr<void const> ptr)
{
    m_buffer = std::make_any<WriteBufferTypes>(std::move(ptr));
    return *this;
}
WriteBuffer const &WriteBuffer::operator=(UniquePtrWithLambda<void> ptr)
{
    m_buffer =
        std::make_any<WriteBufferTypes>(CopyableUniquePtr(std::move(ptr)));
    return *this;
}

void const *WriteBuffer::get() const
{
    return std::visit(
        [](auto const &arg) {
            // unique_ptr and shared_ptr both have the get() member, so
            // we're being sneaky and don't distinguish the types here
            return static_cast<void const *>(arg.get());
        },
        as_variant<WriteBufferTypes>());
}
} // namespace openPMD::auxiliary
