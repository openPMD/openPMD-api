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
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/BaseRecord.hpp"

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
    RecordComponentData::RecordComponentData() = default;
    auto RecordComponentData::push_chunk(IOTask &&task) -> void
    {
        Attributable a;
        a.setData(std::shared_ptr<AttributableData>{this, [](auto const &) {}});
// this check can be too costly in some setups
#if 0
        if (a.containingIteration().closed())
        {
            throw error::WrongAPIUsage(
                "Cannot write/read chunks to/from closed Iterations.");
        }
#endif
        a.setDirtyRecursive(true);
        m_chunks.push(std::move(task));
    }
} // namespace internal

RecordComponent::RecordComponent() : BaseRecordComponent(NoInit())
{
    setData(std::make_shared<Data_t>());
}

RecordComponent::RecordComponent(NoInit) : BaseRecordComponent(NoInit())
{}

RecordComponent::RecordComponent(BaseRecord<RecordComponent> const &baseRecord)
    : BaseRecordComponent(NoInit())
{
    setData(baseRecord.m_recordComponentData);
}

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
        if (!rc.m_dataset.has_value())
        {
            throw error::Internal(
                "Internal control flow error: Written record component must "
                "have defined datatype and extent.");
        }
        if (d.dtype == Datatype::UNDEFINED)
        {
            d.dtype = rc.m_dataset.value().dtype;
        }
        else if (d.dtype != rc.m_dataset.value().dtype)
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
    if (d.empty())
        return makeEmpty(std::move(d));

    rc.m_isEmpty = false;
    if (written())
    {
        rc.m_dataset.value().extend(std::move(d.extent));
    }
    else
    {
        rc.m_dataset = std::move(d);
    }

    setDirty(true);
    return *this;
}

uint8_t RecordComponent::getDimensionality() const
{
    auto &rc = get();
    if (rc.m_dataset.has_value())
    {
        return rc.m_dataset.value().rank;
    }
    else
    {
        return 1;
    }
}

Extent RecordComponent::getExtent() const
{
    auto &rc = get();
    if (rc.m_dataset.has_value())
    {
        return rc.m_dataset.value().extent;
    }
    else
    {
        return {1};
    }
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
        if (!rc.m_dataset.has_value())
        {
            throw error::Internal(
                "Internal control flow error: Written record component must "
                "have defined datatype and extent.");
        }
        if (!constant())
        {
            throw std::runtime_error(
                "An empty record component's extent can only be changed"
                " in case it has been initialized as an empty or constant"
                " record component.");
        }
        if (d.dtype == Datatype::UNDEFINED)
        {
            d.dtype = rc.m_dataset.value().dtype;
        }
        else if (d.dtype != rc.m_dataset.value().dtype)
        {
            throw std::runtime_error(
                "Cannot change the datatype of a dataset.");
        }
        rc.m_dataset.value().extend(std::move(d.extent));
        rc.m_hasBeenExtended = true;
    }
    else
    {
        rc.m_dataset = std::move(d);
    }

    if (rc.m_dataset.value().extent.size() == 0)
        throw std::runtime_error("Dataset extent must be at least 1D.");

    rc.m_isEmpty = true;
    setDirty(true);
    if (!written())
    {
        switchType<detail::DefaultValue<RecordComponent> >(
            rc.m_dataset.value().dtype, *this);
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
    if (access::readOnly(IOHandler()->m_frontendAccess))
    {
        while (!rc.m_chunks.empty())
        {
            IOHandler()->enqueue(rc.m_chunks.front());
            rc.m_chunks.pop();
        }
    }
    else
    {
        /*
         * This catches when a user forgets to use resetDataset.
         */
        if (!rc.m_dataset.has_value())
        {
            // The check for !written() is technically not needed, just
            // defensive programming against internal bugs that go on us.
            if (!written() && rc.m_chunks.empty())
            {
                // No data written yet, just accessed the object so far without
                // doing anything
                // Just do nothing and skip this record component.
                return;
            }
            else
            {
                throw error::WrongAPIUsage(
                    "[RecordComponent] Must specify dataset type and extent "
                    "before flushing (see RecordComponent::resetDataset()).");
            }
        }
        if (!containsAttribute("unitSI"))
        {
            setUnitSI(1);
        }
        if (!written())
        {
            if (constant())
            {
                bool isVBased = retrieveSeries().iterationEncoding() ==
                    IterationEncoding::variableBased;
                Parameter<Operation::CREATE_PATH> pCreate;
                pCreate.path = name;
                IOHandler()->enqueue(IOTask(this, pCreate));
                Parameter<Operation::WRITE_ATT> aWrite;
                aWrite.name = "value";
                aWrite.dtype = rc.m_constantValue.dtype;
                aWrite.resource = rc.m_constantValue.getResource();
                if (isVBased)
                {
                    aWrite.changesOverSteps = Parameter<
                        Operation::WRITE_ATT>::ChangesOverSteps::IfPossible;
                }
                IOHandler()->enqueue(IOTask(this, aWrite));
                aWrite.name = "shape";
                Attribute a(getExtent());
                aWrite.dtype = a.dtype;
                aWrite.resource = a.getResource();
                if (isVBased)
                {
                    aWrite.changesOverSteps = Parameter<
                        Operation::WRITE_ATT>::ChangesOverSteps::IfPossible;
                }
                IOHandler()->enqueue(IOTask(this, aWrite));
            }
            else
            {
                Parameter<Operation::CREATE_DATASET> dCreate;
                dCreate.name = name;
                dCreate.extent = getExtent();
                dCreate.dtype = getDatatype();
                dCreate.options = rc.m_dataset.value().options;
                dCreate.joinedDimension = joinedDimension();
                IOHandler()->enqueue(IOTask(this, dCreate));
            }
        }

        if (rc.m_hasBeenExtended)
        {
            if (constant())
            {
                bool isVBased = retrieveSeries().iterationEncoding() ==
                    IterationEncoding::variableBased;
                Parameter<Operation::WRITE_ATT> aWrite;
                aWrite.name = "shape";
                Attribute a(getExtent());
                aWrite.dtype = a.dtype;
                aWrite.resource = a.getResource();
                if (isVBased)
                {
                    aWrite.changesOverSteps = Parameter<
                        Operation::WRITE_ATT>::ChangesOverSteps::IfPossible;
                }
                IOHandler()->enqueue(IOTask(this, aWrite));
            }
            else
            {
                Parameter<Operation::EXTEND_DATASET> pExtend;
                pExtend.extent = rc.m_dataset.value().extent;
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
    }
    if (flushParams.flushLevel != FlushLevel::SkeletonOnly)
    {
        setDirty(false);
    }
}

void RecordComponent::read(bool require_unit_si)
{
    readBase(require_unit_si);
}

namespace
{
    struct MakeConstant
    {
        template <typename T>
        static void call(RecordComponent rc, Attribute const &attr)
        {
            rc.makeConstant(attr.get<T>());
        }

        template <unsigned n, typename... Args>
        static void call(Args &&...)
        {
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Undefined constant datatype.");
        }
    };
} // namespace

void RecordComponent::readBase(bool require_unit_si)
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
        setWritten(false, Attributable::EnqueueAsynchronously::No);
        switchNonVectorType<MakeConstant>(dtype, *this, a);
        setWritten(true, Attributable::EnqueueAsynchronously::No);

        aRead.name = "shape";
        IOHandler()->enqueue(IOTask(this, aRead));
        IOHandler()->flush(internal::defaultFlushParams);
        a = Attribute(*aRead.resource);
        Extent e;

        // uint64_t check
        if (auto val = a.getOptional<std::vector<uint64_t> >(); val.has_value())
            for (auto const &shape : val.value())
                e.push_back(shape);
        else
        {
            std::ostringstream oss;
            oss << "Unexpected datatype (" << *aRead.dtype
                << ") for attribute 'shape' (" << determineDatatype<uint64_t>()
                << " aka uint64_t)";
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                oss.str());
        }

        setWritten(false, Attributable::EnqueueAsynchronously::No);
        resetDataset(Dataset(dtype, e));
        setWritten(true, Attributable::EnqueueAsynchronously::No);
    }

    readAttributes(ReadMode::FullyReread);

    if (require_unit_si)
    {
        if (!containsAttribute("unitSI"))
        {
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::NotFound,
                {},
                "Attribute unitSI required for record components, not found in "
                "'" +
                    myPath().openPMDPath() + "'.");
        }
        if (!getAttribute("unitSI").getOptional<double>().has_value())
        {
            throw error::ReadError(
                error::AffectedObject::Attribute,
                error::Reason::UnexpectedContent,
                {},
                "Unexpected Attribute datatype for 'unitSI' (expected double, "
                "found " +
                    datatypeToString(Attribute(*aRead.resource).dtype) +
                    ") in '" + myPath().openPMDPath() + "'.");
        }
    }
}

void RecordComponent::storeChunk(
    auxiliary::WriteBuffer buffer, Datatype dtype, Offset o, Extent e)
{
    verifyChunk(dtype, o, e);

    Parameter<Operation::WRITE_DATASET> dWrite;
    dWrite.offset = std::move(o);
    dWrite.extent = std::move(e);
    dWrite.dtype = dtype;
    /* std::static_pointer_cast correctly reference-counts the pointer */
    dWrite.data = std::move(buffer);
    auto &rc = get();
    rc.push_chunk(IOTask(this, std::move(dWrite)));
}

void RecordComponent::verifyChunk(
    Datatype dtype, Offset const &o, Extent const &e) const
{
    if (constant())
        throw std::runtime_error(
            "Chunks cannot be written for a constant RecordComponent.");
    if (empty())
        throw std::runtime_error(
            "Chunks cannot be written for an empty RecordComponent.");
    if (dtype != getDatatype())
    {
        std::ostringstream oss;
        oss << "Datatypes of chunk data (" << dtype
            << ") and record component (" << getDatatype() << ") do not match.";
        throw std::runtime_error(oss.str());
    }
    uint8_t dim = getDimensionality();
    Extent dse = getExtent();

    if (auto jd = joinedDimension(); jd.has_value())
    {
        if (o.size() != 0)
        {
            std::ostringstream oss;
            oss << "Joined array: Must specify an empty offset (given: "
                << "offset=" << o.size() << "D, "
                << "extent=" << e.size() << "D).";
            throw std::runtime_error(oss.str());
        }
        if (e.size() != dim)
        {
            std::ostringstream oss;
            oss << "Joined array: Dimensionalities of chunk extent and dataset "
                   "extent must be equivalent (given: "
                << "offset=" << o.size() << "D, "
                << "extent=" << e.size() << "D).";
            throw std::runtime_error(oss.str());
        }
        for (size_t i = 0; i < dim; ++i)
        {
            if (i != jd.value() && e[i] != dse[i])
            {
                throw std::runtime_error(
                    "Joined array: Chunk extent on non-joined dimensions must "
                    "be equivalent to dataset extents (Dimension on index " +
                    std::to_string(i) + ". DS: " + std::to_string(dse[i]) +
                    " - Chunk: " + std::to_string(o[i] + e[i]) + ")");
            }
        }
    }
    else
    {
        if (e.size() != dim || o.size() != dim)
        {
            std::ostringstream oss;
            oss << "Dimensionality of chunk ("
                << "offset=" << o.size() << "D, "
                << "extent=" << e.size() << "D) "
                << "and record component (" << int(dim) << "D) "
                << "do not match.";
            throw std::runtime_error(oss.str());
        }
        for (uint8_t i = 0; i < dim; ++i)
            if (dse[i] < o[i] + e[i])
                throw std::runtime_error(
                    "Chunk does not reside inside dataset (Dimension on "
                    "index " +
                    std::to_string(i) + ". DS: " + std::to_string(dse[i]) +
                    " - Chunk: " + std::to_string(o[i] + e[i]) + ")");
    }
}

namespace
{
    struct LoadChunkVariant
    {
        template <typename T>
        static RecordComponent::shared_ptr_dataset_types
        call(RecordComponent &rc, Offset o, Extent e)
        {
            return rc.loadChunk<T>(std::move(o), std::move(e));
        }
    };
} // namespace

auto RecordComponent::loadChunkVariant(Offset o, Extent e)
    -> shared_ptr_dataset_types
{
    return visit<LoadChunkVariant>(std::move(o), std::move(e));
}
} // namespace openPMD
