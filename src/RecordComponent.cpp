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
#include "openPMD/Error.hpp"
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
namespace internal
{
    RecordComponentData::RecordComponentData()
    {
        RecordComponent impl{
            std::shared_ptr<RecordComponentData>{this, [](auto const *) {}}};
        impl.setUnitSI(1);
        impl.resetDataset(Dataset(Datatype::CHAR, {1}));
    }
} // namespace internal

RecordComponent::RecordComponent() : BaseRecordComponent{nullptr}
{
    BaseRecordComponent::setData(m_recordComponentData);
}

RecordComponent::RecordComponent(
    std::shared_ptr<internal::RecordComponentData> data)
    : BaseRecordComponent{data}, m_recordComponentData{std::move(data)}
{}

// We need to instantiate this somewhere otherwise there might be linker issues
// despite this thing actually being constepxr
constexpr char const *const RecordComponent::SCALAR;

RecordComponent &RecordComponent::setUnitSI(double usi)
{
    setAttribute("unitSI", usi);
    return *this;
}

RecordComponent &RecordComponent::resetDataset(Dataset d)
{
    auto &rc = get();
    if (written())
    {
        if (d.dtype == Datatype::UNDEFINED)
        {
            d.dtype = rc.m_dataset.dtype;
        }
        else if (d.dtype != rc.m_dataset.dtype)
        {
            throw std::runtime_error(
                "Cannot change the datatype of a dataset.");
        }
        rc.m_hasBeenExtended = true;
    }

    if (d.dtype == Datatype::UNDEFINED)
    {
        throw error::WrongAPIUsage(
            "[RecordComponent] Must set specific datatype.");
    }
    // if( d.extent.empty() )
    //    throw std::runtime_error("Dataset extent must be at least 1D.");
    if (std::any_of(
            d.extent.begin(), d.extent.end(), [](Extent::value_type const &i) {
                return i == 0u;
            }))
        return makeEmpty(std::move(d));

    rc.m_isEmpty = false;
    if (written())
    {
        rc.m_dataset.extend(std::move(d.extent));
    }
    else
    {
        rc.m_dataset = std::move(d);
    }

    dirty() = true;
    return *this;
}

uint8_t RecordComponent::getDimensionality() const
{
    return get().m_dataset.rank;
}

Extent RecordComponent::getExtent() const
{
    return get().m_dataset.extent;
}

namespace detail
{
    struct MakeEmpty
    {
        template <typename T>
        static RecordComponent &call(RecordComponent &rc, uint8_t dimensions)
        {
            return rc.makeEmpty<T>(dimensions);
        }

        template <unsigned int N>
        static RecordComponent &call(RecordComponent &, uint8_t)
        {
            throw std::runtime_error(
                "RecordComponent::makeEmpty: Unknown datatype.");
        }
    };
} // namespace detail

RecordComponent &RecordComponent::makeEmpty(Datatype dt, uint8_t dimensions)
{
    return switchType<detail::MakeEmpty>(dt, *this, dimensions);
}

RecordComponent &RecordComponent::makeEmpty(Dataset d)
{
    auto &rc = get();
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
            d.dtype = rc.m_dataset.dtype;
        }
        else if (d.dtype != rc.m_dataset.dtype)
        {
            throw std::runtime_error(
                "Cannot change the datatype of a dataset.");
        }
        rc.m_dataset.extend(std::move(d.extent));
        rc.m_hasBeenExtended = true;
    }
    else
    {
        rc.m_dataset = std::move(d);
    }

    if (rc.m_dataset.extent.size() == 0)
        throw std::runtime_error("Dataset extent must be at least 1D.");

    rc.m_isEmpty = true;
    dirty() = true;
    if (!written())
    {
        switchType<detail::DefaultValue<RecordComponent> >(
            rc.m_dataset.dtype, *this);
    }
    return *this;
}

bool RecordComponent::empty() const
{
    return get().m_isEmpty;
}

void RecordComponent::flush(
    std::string const &name, internal::FlushParams const &flushParams)
{
    auto &rc = get();
    if (flushParams.flushLevel == FlushLevel::SkeletonOnly)
    {
        rc.m_name = name;
        return;
    }
    switch (IOHandler()->m_frontendAccess)
    {
    case Access::READ_ONLY:
        while (!rc.m_chunks.empty())
        {
            IOHandler()->enqueue(rc.m_chunks.front());
            rc.m_chunks.pop();
        }
        break;
    case Access::READ_WRITE:
    case Access::CREATE:
    case Access::APPEND: {
        /*
         * This catches when a user forgets to use resetDataset.
         */
        if (rc.m_dataset.dtype == Datatype::UNDEFINED)
        {
            throw error::WrongAPIUsage(
                "[RecordComponent] Must set specific datatype (Use "
                "resetDataset call).");
        }
        if (!written())
        {
            if (constant())
            {
                Parameter<Operation::CREATE_PATH> pCreate;
                pCreate.path = name;
                IOHandler()->enqueue(IOTask(this, pCreate));
                Parameter<Operation::WRITE_ATT> aWrite;
                aWrite.name = "value";
                aWrite.dtype = rc.m_constantValue.dtype;
                aWrite.resource = rc.m_constantValue.getResource();
                IOHandler()->enqueue(IOTask(this, aWrite));
                aWrite.name = "shape";
                Attribute a(getExtent());
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
                dCreate.options = rc.m_dataset.options;
                IOHandler()->enqueue(IOTask(this, dCreate));
            }
        }

        if (rc.m_hasBeenExtended)
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
                pExtend.extent = rc.m_dataset.extent;
                IOHandler()->enqueue(IOTask(this, std::move(pExtend)));
                rc.m_hasBeenExtended = false;
            }
        }

        while (!rc.m_chunks.empty())
        {
            IOHandler()->enqueue(rc.m_chunks.front());
            rc.m_chunks.pop();
        }

        flushAttributes(flushParams);
        break;
    }
    }
}

void RecordComponent::read()
{
    readBase();
}

void RecordComponent::readBase()
{
    using DT = Datatype;
    // auto & rc = get();
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
    return !get().m_chunks.empty();
}
} // namespace openPMD
