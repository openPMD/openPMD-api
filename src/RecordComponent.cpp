/* Copyright 2017-2025 Fabian Koller, Axel Huebl, Franz Poeschel, Junmin Gu
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
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/Format.hpp"
#include "openPMD/LoadStoreChunk.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/Variant_internal.hpp"
#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults.hpp"

// comment so clang-format does not move this
#include "openPMD/DatatypeMacros.hpp"

// comment
#include "openPMD/DatatypeMacros.hpp"

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
#if openPMD_USE_INVASIVE_TESTS

        auto maybe_an_iteration = a.containingIteration().first;
        if (!maybe_an_iteration.has_value())
        {
            throw std::runtime_error(
                "Trying to write to/read from a RecordComponent that is not "
                "contained by any Iteration.");
        }
        auto &iterationData = *maybe_an_iteration.value();
        auto iteration = iterationData.asInternalCopyOf<Iteration>();
        if (iteration.closed() && !iterationData.allow_reopening_implicitly)
        {
            throw std::runtime_error(
                "Cannot write/read chunks to/from closed Iterations.");
        }
#endif
        a.setDirtyRecursive(true);
        m_chunks.push(std::move(task));
    }

    static constexpr char const *note_on_deactivating_this_check = R"(
Note: In order to ignore inconsistent / incomplete extent definitions,
set the environment variable OPENPMD_VERIFY_HOMOGENEOUS_EXTENTS=0
or alternatively the JSON option {"verify_homogeneous_extents": false}.
    )";

    HomogenizeExtents::HomogenizeExtents() = default;
    HomogenizeExtents::HomogenizeExtents(bool verify_homogeneous_extents_in)
        : verify_homogeneous_extents(verify_homogeneous_extents_in)
    {}

    void HomogenizeExtents::check_extent(
        Attributable const &callsite, RecordComponent &rc)
    {
        auto extent = rc.getExtent();
        if (Dataset::undefinedExtent(extent))
        {
            without_extent.emplace_back(rc);
        }
        else if (retrieved_extent.has_value())
        {
            if (verify_homogeneous_extents && extent != *retrieved_extent)
            {
                std::stringstream error_msg;
                error_msg << "Inconsistent extents found for Record '"
                          << callsite.myPath().openPMDPath() << "': Component '"
                          << rc.myPath().openPMDPath() << "' has extent";
                auxiliary::write_vec_to_stream(error_msg, extent) << ", but ";
                auxiliary::write_vec_to_stream(error_msg, *retrieved_extent)
                    << " was found previously."
                    << note_on_deactivating_this_check;
                throw error::ReadError(
                    error::AffectedObject::Group,
                    error::Reason::UnexpectedContent,
                    std::nullopt,
                    error_msg.str());
            }
        }
        else
        {
            retrieved_extent = std::move(extent);
        }
    }

    auto HomogenizeExtents::merge(
        Attributable const &callsite, HomogenizeExtents other)
        -> HomogenizeExtents &
    {
        if (retrieved_extent.has_value() && other.retrieved_extent.has_value())
        {
            if (verify_homogeneous_extents &&
                *retrieved_extent != *other.retrieved_extent)
            {
                std::stringstream error_msg;
                error_msg << "Inconsistent extents found for Record '"
                          << callsite.myPath().openPMDPath() << "': ";
                auxiliary::write_vec_to_stream(error_msg, *retrieved_extent)
                    << " vs. ";
                auxiliary::write_vec_to_stream(
                    error_msg, *other.retrieved_extent)
                    << "." << note_on_deactivating_this_check;
                throw error::ReadError(
                    error::AffectedObject::Group,
                    error::Reason::UnexpectedContent,
                    std::nullopt,
                    error_msg.str());
            }
        }
        else if (!retrieved_extent.has_value())
        {
            retrieved_extent = std::move(other.retrieved_extent);
        }

        for (auto &rc : other.without_extent)
        {
            this->without_extent.emplace_back(std::move(rc));
        }
        return *this;
    }

    void HomogenizeExtents::homogenize(Attributable const &callsite) &&
    {
        if (!retrieved_extent.has_value())
        {
            if (verify_homogeneous_extents)
            {
                throw error::ReadError(
                    error::AffectedObject::Group,
                    error::Reason::UnexpectedContent,
                    std::nullopt,
                    "No extent found for any component contained in '" +
                        callsite.myPath().openPMDPath() + "'." +
                        note_on_deactivating_this_check);
            }
            else
            {
                return;
            }
        }
        auto &ext = *retrieved_extent;
        for (auto &rc : without_extent)
        {
            rc.setWritten(false, Attributable::EnqueueAsynchronously::No);
            rc.resetDataset(Dataset(Datatype::UNDEFINED, ext));
            rc.setWritten(true, Attributable::EnqueueAsynchronously::No);
        }
        without_extent.clear();
    }

} // namespace internal

template <typename T>
auto resource(T &t) -> attribute_types &
{
    return t.template resource<attribute_types>();
}

ConfigureLoadStore RecordComponent::prepareLoadStore()
{
    return ConfigureLoadStore{*this};
}

namespace
{
#if (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 11000) ||                   \
    (defined(__apple_build_version__) && __clang_major__ < 14)
    template <typename T>
    auto createSpanBufferFallback(size_t size) -> UniquePtrWithLambda<T>
    {
        return UniquePtrWithLambda<T>{
            new T[size], [](auto *ptr) { delete[] ptr; }};
    }
#else
    template <typename T>
    auto createSpanBufferFallback(size_t size) -> std::unique_ptr<T[]>
    {
        return std::unique_ptr<T[]>{new T[size]};
    }
#endif
} // namespace

template <typename T>
DynamicMemoryView<T>
RecordComponent::storeChunkSpan_impl(internal::LoadStoreConfig cfg)
{
    return storeChunkSpanCreateBuffer_impl<T>(
        std::move(cfg), &createSpanBufferFallback<T>);
}

template <typename T_with_extent>
std::shared_ptr<T_with_extent>
RecordComponent::loadChunkAllocate_impl(internal::LoadStoreConfig cfg)
{
    using T = std::remove_cv_t<std::remove_extent_t<T_with_extent>>;
    auto res = loadChunkAllocate_impl(
        determineDatatype<T>(), sizeof(T), std::move(cfg));
    return std::static_pointer_cast<T_with_extent>(res);
}

std::shared_ptr<void> RecordComponent::loadChunkAllocate_impl(
    Datatype dtype, size_t dtype_size, internal::LoadStoreConfig cfg)
{
    auto [o, e] = std::move(cfg);

    size_t numPoints = 1;
    for (auto val : e)
    {
        numPoints *= val;
    }

    auto newData =
        std::shared_ptr<void>(new char[numPoints * dtype_size], [](void *p) {
            delete[] (static_cast<char *>(p));
        });
    prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .withSharedPtr_impl_mut(newData, dtype)
        .load(EnqueuePolicy::Defer);
    return newData;
}

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

    /*
     * For backwards-compatibility reasons, we do not actually run the below
     * flush. But the API should be written in such a way that the entire test
     * suite passes when enabling the below code block.
     */
#if 0
    auto cleanup = auxiliary::defer([&rc, this]() {
        if (rc.m_dataset.has_value() &&
            rc.m_dataset->dtype != Datatype::UNDEFINED &&
            IOHandler()->m_seriesStatus != internal::SeriesStatus::Parsing)
        {
            seriesFlush_impl</* flush_entire_series = */ false>(
                {FlushLevel::SkeletonOnly}, /* flush_io_handler = */ true);
        }
    });
#endif

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

    if (d.extent.empty())
        throw std::runtime_error("Dataset extent must be at least 1D.");
    if (d.empty())
    {
        if (d.extent.empty())
        {
            throw error::Internal(
                "A zero-dimensional dataset is not to be considered empty, but "
                "undefined. This error is an internal safeguard against future "
                "changes that might not consider this.");
        }
        else if (d.dtype != Datatype::UNDEFINED)
        {
            setDirty(true);
            return makeEmpty(std::move(d));
        }
        else
        {
            rc.m_dataset = std::move(d);
            setDirty(true);
            return *this;
        }
    }

    rc.m_isEmpty = false;
    if (written())
    {
        if (!rc.m_dataset.has_value())
        {
            throw error::Internal(
                "Internal control flow error: Written record component must "
                "have defined datatype and extent.");
        }
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
        return {Dataset::UNDEFINED_EXTENT};
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
        switchType<detail::DefaultValue<RecordComponent>>(
            rc.m_dataset.value().dtype, *this);
    }
    return *this;
}

bool RecordComponent::empty() const
{
    return get().m_isEmpty;
}

void RecordComponent::visitHierarchy(HierarchyVisitor &v, bool)
{
    v(*this);
}

void RecordComponent::flush(
    std::string const &name, internal::FlushParams const &flushParams)
{
    if (!dirtyRecursive())
    {
        return;
    }
    auto &rc = get();
    if (flushParams.flushLevel == FlushLevel::SkeletonOnly)
    {
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
            if (!written() && rc.m_chunks.empty() && !rc.m_isConstant)
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
                    "before flushing or setting a constant value (see "
                    "RecordComponent::resetDataset()).");
            }
        }
        auto constant_component_write_shape = [&]() {
            auto extent = getExtent();
            return !Dataset::undefinedExtent(extent) &&
                std::none_of(extent.begin(), extent.end(), [](auto val) {
                    return val == Dataset::JOINED_DIMENSION;
                });
        };
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
                aWrite.m_resource = rc.m_constantValue.getAny();
                if (isVBased)
                {
                    aWrite.changesOverSteps = Parameter<
                        Operation::WRITE_ATT>::ChangesOverSteps::IfPossible;
                }
                IOHandler()->enqueue(IOTask(this, aWrite));
                if (constant_component_write_shape())
                {
                    aWrite.name = "shape";
                    Attribute a(getExtent());
                    aWrite.dtype = a.dtype;
                    aWrite.m_resource = a.getAny();
                    if (isVBased)
                    {
                        aWrite.changesOverSteps = Parameter<
                            Operation::WRITE_ATT>::ChangesOverSteps::IfPossible;
                    }
                    IOHandler()->enqueue(IOTask(this, aWrite));
                }
            }
            else
            {
                Parameter<Operation::CREATE_DATASET> dCreate(
                    rc.m_dataset.value());
                dCreate.name = name;
                IOHandler()->enqueue(IOTask(this, dCreate));
            }
        }

        if (rc.m_hasBeenExtended)
        {
            if (constant())
            {
                if (!constant_component_write_shape())
                {
                    throw error::WrongAPIUsage(
                        "Extended constant component from a previous shape to "
                        "one that cannot be written (empty or with joined "
                        "dimension).");
                }
                bool isVBased = retrieveSeries().iterationEncoding() ==
                    IterationEncoding::variableBased;
                Parameter<Operation::WRITE_ATT> aWrite;
                aWrite.name = "shape";
                Attribute a(getExtent());
                aWrite.dtype = a.dtype;
                aWrite.m_resource = a.getAny();
                if (isVBased)
                {
                    aWrite.changesOverSteps = Parameter<
                        Operation::WRITE_ATT>::ChangesOverSteps::IfPossible;
                }
                IOHandler()->enqueue(IOTask(this, aWrite));
            }
            else
            {
                Parameter<Operation::EXTEND_DATASET> pExtend(
                    rc.m_dataset.value().extent);
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
    determineUnsetDirty(flushParams.flushLevel);
}

void RecordComponent::read()
{
    readBase();
    internal::ScientificDefaults::readDefaults(*this, IOHandler()->m_standard);
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

void RecordComponent::readBase()
{
    using DT = Datatype;
    auto &rc = get();

    readAttributes(ReadMode::FullyReread);

    auto read_constant = [&]() {
        Attribute a = rc.readAttribute("value");
        DT dtype = a.dtype;
        setWritten(false, Attributable::EnqueueAsynchronously::No);
        switchNonVectorType<MakeConstant>(dtype, *this, a);
        setWritten(true, Attributable::EnqueueAsynchronously::No);

        if (!containsAttribute("shape"))
        {
            setWritten(false, Attributable::EnqueueAsynchronously::No);
            resetDataset(Dataset(dtype, {Dataset::UNDEFINED_EXTENT}));
            setWritten(true, Attributable::EnqueueAsynchronously::No);

            return;
        }

        a = rc.attributes().at("shape");
        Extent e;

        // uint64_t check
        if (auto val = a.getOptional<std::vector<uint64_t>>(); val.has_value())
            for (auto const &shape : val.value())
                e.push_back(shape);
        else
        {
            std::ostringstream oss;
            oss << "Unexpected datatype (" << a.dtype
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
    };

    if (constant() && !empty())
    {
        read_constant();
    }
}

void RecordComponent::storeChunk_impl(
    auxiliary::WriteBuffer buffer,
    Datatype dtype,
    internal::LoadStoreConfigWithBuffer cfg)
{
    auto [o, e, memorySelection] = std::move(cfg);
    verifyChunk(dtype, o, e);

    Parameter<Operation::WRITE_DATASET> dWrite;
    dWrite.offset = std::move(o);
    dWrite.extent = std::move(e);
    dWrite.memorySelection = memorySelection;
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
    if (!isSame(dtype, getDatatype()))
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

void RecordComponent::scientificDefaults_impl(
    internal::WriteOrRead wor, OpenpmdStandard)
{
    using namespace internal;
    auto numerical_types = get_numerical_types();

    defaultAttribute(*this, "unitSI")
        .template withSetter<RecordComponent>(1.0, &RecordComponent::setUnitSI)
        .withReader(numerical_types, require_type<double>())(wor);
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

template <typename T>
RecordComponent &RecordComponent::makeConstant(T value)
{
    if (written())
        throw std::runtime_error(
            "A recordComponent can not (yet) be made constant after it has "
            "been written.");

    auto &rc = get();

    rc.m_constantValue = Attribute(value);
    rc.m_isConstant = true;
    return *this;
}

template <typename T>
RecordComponent &RecordComponent::makeEmpty(uint8_t dimensions)
{
    return makeEmpty(Dataset(determineDatatype<T>(), Extent(dimensions, 0)));
}

template <typename T>
std::shared_ptr<T> RecordComponent::loadChunk(Offset o, Extent e)
{
    uint8_t dim = getDimensionality();
    auto operation = prepareLoadStore();

    // default arguments
    //   offset = {0u}: expand to right dim {0u, 0u, ...}
    if (o.size() != 1u || o.at(0) != 0u || dim <= 1u)
    {
        operation.offset(std::move(o));
    }

    //   extent = {-1u}: take full size
    if (e.size() != 1u || e.at(0) != -1u)
    {
        operation.extent(std::move(e));
    }

    return operation.load<T>(EnqueuePolicy::Defer);
}

namespace detail
{
    struct FillBuffer
    {
        template <typename T>
        static void call(
            void *target,
            size_t numPoints,
            RecordComponent const &component,
            internal::RecordComponentData const &rc)
        {
            std::optional<T> val = rc.m_constantValue.getOptional<T>();

            if (val.has_value())
            {
                auto raw_ptr = static_cast<T *>(target);
                std::fill(raw_ptr, raw_ptr + numPoints, *val);
            }
            else
            {
                std::string const data_type_str =
                    datatypeToString(component.getDatatype());
                std::string const requ_type_str =
                    datatypeToString(determineDatatype<T>());
                std::string err_msg =
                    "Type conversion during chunk loading not possible! ";
                err_msg +=
                    "Data: " + data_type_str + "; Load as: " + requ_type_str;
                throw error::WrongAPIUsage(err_msg);
            }
        }

        static constexpr char const *errorMsg = "FillBuffer";
    };
} // namespace detail

template <typename T>
void RecordComponent::loadChunk_impl(
    std::shared_ptr<T> const &data, internal::LoadStoreConfigWithBuffer cfg)
{
    loadChunk_impl(
        std::static_pointer_cast<void>(data),
        determineDatatype<std::remove_cv_t<std::remove_extent_t<T>>>(),
        std::move(cfg));
}

void RecordComponent::loadChunk_impl(
    std::shared_ptr<void> const &data,
    Datatype dtype_requested,
    internal::LoadStoreConfigWithBuffer cfg)
{
    if (cfg.memorySelection.has_value())
    {
        throw error::WrongAPIUsage(
            "Unsupported: Memory selections in chunk loading.");
    }
    /*
     * For constant components, we implement type conversion, so there is
     * a separate check further below.
     * This is especially useful for the short-attribute representation in the
     * JSON/TOML backends as they might implicitly turn a LONG into an INT in a
     * constant component. The frontend needs to catch such edge cases.
     * Ref. `if (constant())` branch.
     *
     * Attention: Do NOT use operator==(), doesnt work properly on Windows!
     */
    if (!isSame(dtype_requested, getDatatype()) && !constant())
    {
        std::string const data_type_str = datatypeToString(getDatatype());
        std::string const requ_type_str = datatypeToString(dtype_requested);
        std::string err_msg =
            "Type conversion during chunk loading not yet implemented! ";
        err_msg += "Data: " + data_type_str + "; Load as: " + requ_type_str;
        throw std::runtime_error(err_msg);
    }

    auto dim = getDimensionality();
    auto [offset, extent, memorySelection] = std::move(cfg);

    if (extent.size() != dim || offset.size() != dim)
    {
        std::ostringstream oss;
        oss << "Dimensionality of chunk ("
            << "offset=" << offset.size() << "D, "
            << "extent=" << extent.size() << "D) "
            << "and record component (" << int(dim) << "D) "
            << "do not match.";
        throw std::runtime_error(oss.str());
    }
    Extent dse = getExtent();
    for (uint8_t i = 0; i < dim; ++i)
        if (dse[i] < offset[i] + extent[i])
            throw std::runtime_error(
                "Chunk does not reside inside dataset (Dimension on index " +
                std::to_string(i) + ". DS: " + std::to_string(dse[i]) +
                " - Chunk: " + std::to_string(offset[i] + extent[i]) + ")");

    auto &rc = get();
    if (constant())
    {
        uint64_t numPoints = 1u;
        for (auto const &dimensionSize : extent)
            numPoints *= dimensionSize;

        switchDatasetType<detail::FillBuffer>(
            dtype_requested, data.get(), numPoints, *this, rc);
    }
    else
    {
        Parameter<Operation::READ_DATASET> dRead;
        dRead.offset = offset;
        dRead.extent = extent;
        dRead.dtype = getDatatype();
        dRead.data = std::static_pointer_cast<void>(data);
        rc.push_chunk(IOTask(this, dRead));
    }
}

template <typename T>
void RecordComponent::loadChunk(std::shared_ptr<T> data, Offset o, Extent e)
{
    // static_assert(!std::is_same_v<T_with_extent, std::string>, "EVIL");
    uint8_t dim = getDimensionality();
    auto operation = prepareLoadStore();

    // default arguments
    //   offset = {0u}: expand to right dim {0u, 0u, ...}
    if (o.size() != 1u || o.at(0) != 0u || dim <= 1u)
    {
        operation.offset(std::move(o));
    }

    //   extent = {-1u}: take full size
    if (e.size() != 1u || e.at(0) != -1u)
    {
        operation.extent(std::move(e));
    }

    operation.withSharedPtr(std::move(data)).load(EnqueuePolicy::Defer);
}

template <typename T>
void RecordComponent::loadChunkRaw(T *ptr, Offset offset, Extent extent)
{
    prepareLoadStore()
        .offset(std::move(offset))
        .extent(std::move(extent))
        .withRawPtr(ptr)
        .load(EnqueuePolicy::Defer);
}

template <typename T>
void RecordComponent::storeChunk(std::shared_ptr<T> data, Offset o, Extent e)
{
    prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .withSharedPtr(std::move(data))
        .store(EnqueuePolicy::Defer);
}

template <typename T>
void RecordComponent::storeChunk(
    UniquePtrWithLambda<T> data, Offset o, Extent e)
{
    prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .withUniquePtr(std::move(data))
        .store(EnqueuePolicy::Defer);
}

template <typename T>
void RecordComponent::storeChunkRaw(T const *ptr, Offset offset, Extent extent)
{
    prepareLoadStore()
        .offset(std::move(offset))
        .extent(std::move(extent))
        .withRawPtr(ptr)
        .store(EnqueuePolicy::Defer);
}

template <typename T>
DynamicMemoryView<T> RecordComponent::storeChunk(Offset offset, Extent extent)
{
    return prepareLoadStore()
        .offset(std::move(offset))
        .extent(std::move(extent))
        .enqueueStore<T>();
}

template <typename T>
void RecordComponent::verifyChunk(Offset const &o, Extent const &e) const
{
    verifyChunk(determineDatatype<T>(), o, e);
}

// Needed for clang-tidy's peace of mind.
#define OPENPMD_PTR(type) type *
#define OPENPMD_ARRAY(type) type[]

#define OPENPMD_INSTANTIATE_BASIC(type)                                        \
    template void RecordComponent::loadChunkRaw<type>(                         \
        OPENPMD_PTR(type) ptr, Offset offset, Extent extent);                  \
    template void RecordComponent::verifyChunk<type>(                          \
        Offset const &o, Extent const &e) const;                               \
    template DynamicMemoryView<type> RecordComponent::storeChunk<type>(        \
        Offset offset, Extent extent);                                         \
    template void RecordComponent::storeChunkRaw<type>(                        \
        OPENPMD_PTR(type const) ptr, Offset offset, Extent extent);            \
    template DynamicMemoryView<type> RecordComponent::storeChunkSpan_impl(     \
        internal::LoadStoreConfig cfg);

#define OPENPMD_INSTANTIATE_CONST_AND_NONCONST(type)

#define OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT(type)                      \
    template void RecordComponent::loadChunk<type>(                            \
        std::shared_ptr<type> data, Offset o, Extent e);                       \
    template std::shared_ptr<type> RecordComponent::loadChunk<type>(           \
        Offset o, Extent e);                                                   \
    template void RecordComponent::storeChunk<type>(                           \
        UniquePtrWithLambda<type> data, Offset o, Extent e);                   \
    template void RecordComponent::loadChunk_impl(                             \
        std::shared_ptr<type> const &data,                                     \
        internal::LoadStoreConfigWithBuffer cfg);                              \
    template std::shared_ptr<type> RecordComponent::loadChunkAllocate_impl(    \
        internal::LoadStoreConfig cfg);

#define OPENPMD_INSTANTIATE_FULLMATRIX(type)                                   \
    template void RecordComponent::storeChunk<type>(                           \
        std::shared_ptr<type> data, Offset o, Extent e);                       \
    template RecordComponent &RecordComponent::makeConstant<type>(type);       \
    template RecordComponent &RecordComponent::makeEmpty<type>(                \
        uint8_t dimensions);

#define OPENPMD_INSTANTIATE(type)                                              \
    OPENPMD_INSTANTIATE_BASIC(type)                                            \
    OPENPMD_INSTANTIATE_CONST_AND_NONCONST(type)                               \
    OPENPMD_INSTANTIATE_CONST_AND_NONCONST(type const)                         \
    OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT(type)                          \
    OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT(OPENPMD_ARRAY(type))           \
    OPENPMD_INSTANTIATE_FULLMATRIX(type)                                       \
    OPENPMD_INSTANTIATE_FULLMATRIX(type const)                                 \
    OPENPMD_INSTANTIATE_FULLMATRIX(OPENPMD_ARRAY(type))                        \
    OPENPMD_INSTANTIATE_FULLMATRIX(type const[])

OPENPMD_FOREACH_DATASET_DATATYPE(OPENPMD_INSTANTIATE)
#undef OPENPMD_INSTANTIATE
#undef OPENPMD_INSTANTIATE_FULLMATRIX
#undef OPENPMD_INSTANTIATE_WITH_AND_WITHOUT_EXTENT
#undef OPENPMD_INSTANTIATE_CONST_AND_NONCONST
#undef OPENPMD_INSTANTIATE_BASIC
#undef OPENPMD_PTR
#undef OPENPMD_ARRAY

} // namespace openPMD
