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
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Dataset.hpp"
#include "openPMD/DatatypeHelpers.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/Memory.hpp"

#include <algorithm>
#include <climits>
#include <complex>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace openPMD
{
// We need to instantiate this somewhere otherwise there might be linker issues
// despite this thing actually being constepxr
constexpr char const *const RecordComponent::SCALAR;

RecordComponent::RecordComponent()
    : m_chunks{std::make_shared<std::queue<IOTask> >()}
    , m_constantValue{std::make_shared<Attribute>(-1)}
{
    setUnitSI(1);
    resetDataset(Dataset(Datatype::CHAR, {1}));
}

RecordComponent &RecordComponent::setUnitSI(double usi)
{
    setAttribute("unitSI", usi);
    return *this;
}

RecordComponent &RecordComponent::resetDataset(Dataset d)
{
    if (written())
    {
        if (d.dtype == Datatype::UNDEFINED)
        {
            d.dtype = m_dataset->dtype;
        }
        else if (d.dtype != m_dataset->dtype)
        {
            throw std::runtime_error(
                "Cannot change the datatype of a dataset.");
        }
        *m_hasBeenExtended = true;
    }
    // if( d.extent.empty() )
    //    throw std::runtime_error("Dataset extent must be at least 1D.");
    if (std::any_of(
            d.extent.begin(), d.extent.end(), [](Extent::value_type const &i) {
                return i == 0u;
            }))
        return makeEmpty(std::move(d));

    *m_isEmpty = false;
    if (written())
    {
        m_dataset->extend(std::move(d.extent));
    }
    else
    {
        *m_dataset = std::move(d);
    }

    dirty() = true;
    return *this;
}

uint8_t RecordComponent::getDimensionality() const
{
    return m_dataset->rank;
}

Extent RecordComponent::getExtent() const
{
    return m_dataset->extent;
}

namespace detail
{
    struct MakeEmpty
    {
        template <typename T>
        RecordComponent &operator()(RecordComponent &rc, uint8_t dimensions)
        {
            return rc.makeEmpty<T>(dimensions);
        }

        template <unsigned int N>
        RecordComponent &operator()(RecordComponent &, uint8_t)
        {
            throw std::runtime_error(
                "RecordComponent::makeEmpty: Unknown datatype.");
        }
    };
} // namespace detail

RecordComponent &RecordComponent::makeEmpty(Datatype dt, uint8_t dimensions)
{
    static detail::MakeEmpty me;
    return switchType(dt, me, *this, dimensions);
}

RecordComponent &RecordComponent::makeEmpty(Dataset d)
{
    if (written())
    {
        if (!constant())
        {
            throw std::runtime_error(
                "An empty record component's extent can only be changed"
                " in case it has been initialized as an empty or constant"
                " record component.");
        }
        if (d.dtype == Datatype::UNDEFINED)
        {
            d.dtype = m_dataset->dtype;
        }
        else if (d.dtype != m_dataset->dtype)
        {
            throw std::runtime_error(
                "Cannot change the datatype of a dataset.");
        }
        m_dataset->extend(std::move(d.extent));
        *m_hasBeenExtended = true;
    }
    else
    {
        *m_dataset = std::move(d);
    }

    if (m_dataset->extent.size() == 0)
        throw std::runtime_error("Dataset extent must be at least 1D.");

    *m_isEmpty = true;
    dirty() = true;
    if (!written())
    {
        static detail::DefaultValue<RecordComponent> dv;
        switchType(m_dataset->dtype, dv, *this);
    }
    return *this;
}

bool RecordComponent::empty() const
{
    return *m_isEmpty;
}

void RecordComponent::flush(
    std::string const &name, internal::FlushParams const &flushParams)
{
    if (flushParams.flushLevel == FlushLevel::SkeletonOnly)
    {
        *this->m_name = name;
        return;
    }
    if (IOHandler()->m_frontendAccess == Access::READ_ONLY)
    {
        while (!m_chunks->empty())
        {
            IOHandler()->enqueue(m_chunks->front());
            m_chunks->pop();
        }
    }
    else
    {
        if (!written())
        {
            if (constant())
            {
                Parameter<Operation::CREATE_PATH> pCreate;
                pCreate.path = name;
                IOHandler()->enqueue(IOTask(this, pCreate));
                Parameter<Operation::WRITE_ATT> aWrite;
                aWrite.name = "value";
                aWrite.dtype = m_constantValue->dtype;
                aWrite.resource = m_constantValue->getResource();
                IOHandler()->enqueue(IOTask(this, aWrite));
                aWrite.name = "shape";
                // note: due to a C++17 issue with ICC 19.1.2 we write the
                //       T value to variant conversion explicitly
                //       https://github.com/openPMD/openPMD-api/pull/...
                // Attribute a(getExtent());
                Attribute a(static_cast<Attribute::resource>(getExtent()));
                aWrite.dtype = a.dtype;
                aWrite.resource = a.getResource();
                IOHandler()->enqueue(IOTask(this, aWrite));
            }
            else
            {
                Parameter<Operation::CREATE_DATASET> dCreate;
                dCreate.name = name;
                dCreate.extent = getExtent();
                dCreate.dtype = getDatatype();
                dCreate.chunkSize = m_dataset->chunkSize;
                dCreate.compression = m_dataset->compression;
                dCreate.transform = m_dataset->transform;
                dCreate.options = m_dataset->options;
                IOHandler()->enqueue(IOTask(this, dCreate));
            }
        }

        if (*m_hasBeenExtended)
        {
            if (constant())
            {
                Parameter<Operation::WRITE_ATT> aWrite;
                aWrite.name = "shape";
                Attribute a(getExtent());
                aWrite.dtype = a.dtype;
                aWrite.resource = a.getResource();
                IOHandler()->enqueue(IOTask(this, aWrite));
            }
            else
            {
                Parameter<Operation::EXTEND_DATASET> pExtend;
                pExtend.extent = m_dataset->extent;
                IOHandler()->enqueue(IOTask(this, std::move(pExtend)));
                *m_hasBeenExtended = false;
            }
        }

        while (!m_chunks->empty())
        {
            IOHandler()->enqueue(m_chunks->front());
            m_chunks->pop();
        }

        flushAttributes(flushParams);
    }
}

void RecordComponent::read()
{
    readBase();
}

void RecordComponent::readBase()
{
    using DT = Datatype;
    Parameter<Operation::READ_ATT> aRead;

    if (constant() && !empty())
    {
        aRead.name = "value";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);

        Attribute a(*aRead.resource);
        DT dtype = *aRead.dtype;
        written() = false;
        switch (dtype)
        {
        case DT::LONG_DOUBLE:
            makeConstant(a.get<long double>());
            break;
        case DT::DOUBLE:
            makeConstant(a.get<double>());
            break;
        case DT::FLOAT:
            makeConstant(a.get<float>());
            break;
        case DT::CLONG_DOUBLE:
            makeConstant(a.get<std::complex<long double> >());
            break;
        case DT::CDOUBLE:
            makeConstant(a.get<std::complex<double> >());
            break;
        case DT::CFLOAT:
            makeConstant(a.get<std::complex<float> >());
            break;
        case DT::SHORT:
            makeConstant(a.get<short>());
            break;
        case DT::INT:
            makeConstant(a.get<int>());
            break;
        case DT::LONG:
            makeConstant(a.get<long>());
            break;
        case DT::LONGLONG:
            makeConstant(a.get<long long>());
            break;
        case DT::USHORT:
            makeConstant(a.get<unsigned short>());
            break;
        case DT::UINT:
            makeConstant(a.get<unsigned int>());
            break;
        case DT::ULONG:
            makeConstant(a.get<unsigned long>());
            break;
        case DT::ULONGLONG:
            makeConstant(a.get<unsigned long long>());
            break;
        case DT::CHAR:
            makeConstant(a.get<char>());
            break;
        case DT::UCHAR:
            makeConstant(a.get<unsigned char>());
            break;
        case DT::BOOL:
            makeConstant(a.get<bool>());
            break;
        default:
            throw std::runtime_error("Unexpected constant datatype");
        }
        written() = true;

        aRead.name = "shape";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);
        a = Attribute(*aRead.resource);
        Extent e;

        // uint64_t check
        Datatype const attrDtype = *aRead.dtype;
        if (isSame(attrDtype, determineDatatype<std::vector<uint64_t> >()) ||
            isSame(attrDtype, determineDatatype<uint64_t>()))
            for (auto const &val : a.get<std::vector<uint64_t> >())
                e.push_back(val);
        else
        {
            std::ostringstream oss;
            oss << "Unexpected datatype (" << *aRead.dtype
                << ") for attribute 'shape' (" << determineDatatype<uint64_t>()
                << " aka uint64_t)";
            throw std::runtime_error(oss.str());
        }

        written() = false;
        resetDataset(Dataset(dtype, e));
        written() = true;
    }

    aRead.name = "unitSI";
    IOHandler()->enqueue(IOTask(this, aRead));
    IOHandler()->flush(internal::defaultFlushParams);
    if (*aRead.dtype == DT::DOUBLE)
        setUnitSI(Attribute(*aRead.resource).get<double>());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'unitSI'");

    readAttributes(ReadMode::FullyReread);
}

bool RecordComponent::dirtyRecursive() const
{
    if (this->dirty())
    {
        return true;
    }
    return !m_chunks->empty();
}
} // namespace openPMD
