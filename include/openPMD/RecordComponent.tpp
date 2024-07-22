/* Copyright 2017-2021 Fabian Koller, Axel Huebl and Franz Poeschel
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

#include "openPMD/Error.hpp"
#include "openPMD/LoadStoreChunk.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Span.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/ShareRawInternal.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

#include <memory>
#include <optional>
#include <type_traits>

namespace openPMD
{
template <typename T>
inline RecordComponent &RecordComponent::makeConstant(T value)
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
inline RecordComponent &RecordComponent::makeEmpty(uint8_t dimensions)
{
    return makeEmpty(Dataset(determineDatatype<T>(), Extent(dimensions, 0)));
}

template <typename T>
inline std::shared_ptr<T> RecordComponent::loadChunk(Offset o, Extent e)
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

template <typename T>
inline std::shared_ptr<T>
RecordComponent::loadChunkAllocate_impl(internal::LoadStoreConfig cfg)
{
    static_assert(!std::is_same_v<T, std::string>, "EVIL");
    auto [o, e] = std::move(cfg);

    size_t numPoints = 1;
    for (auto val : e)
    {
        numPoints *= val;
    }

#if (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 11000) ||                   \
    (defined(__apple_build_version__) && __clang_major__ < 14)
    auto newData =
        std::shared_ptr<T>(new T[numPoints], [](T *p) { delete[] p; });
    prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .withSharedPtr(newData)
        .load(EnqueuePolicy::Defer);
    return newData;
#else
    auto newData = std::shared_ptr<T[]>(new T[numPoints]);
    prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .withSharedPtr(newData)
        .load(EnqueuePolicy::Defer);
    return std::static_pointer_cast<T>(std::move(newData));
#endif
}

template <typename T_with_extent>
inline void RecordComponent::loadChunk(
    std::shared_ptr<T_with_extent> data, Offset o, Extent e)
{
    static_assert(!std::is_same_v<T_with_extent, std::string>, "EVIL");
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

template <typename T_with_extent>
inline void RecordComponent::loadChunk_impl(
    std::shared_ptr<T_with_extent> data,
    internal::LoadStoreConfigWithBuffer cfg)
{
    if (cfg.memorySelection.has_value())
    {
        throw error::WrongAPIUsage(
            "Unsupported: Memory selections in chunk loading.");
    }
    using T = std::remove_cv_t<std::remove_extent_t<T_with_extent>>;
    Datatype dtype = determineDatatype(data);
    if (dtype != getDatatype())
        if (!isSameInteger<T>(getDatatype()) &&
            !isSameFloatingPoint<T>(getDatatype()) &&
            !isSameComplexFloatingPoint<T>(getDatatype()) &&
            !isSameChar<T>(getDatatype()))
        {
            std::string const data_type_str = datatypeToString(getDatatype());
            std::string const requ_type_str =
                datatypeToString(determineDatatype<T>());
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

        T value = rc.m_constantValue.get<T>();

        auto raw_ptr = static_cast<T *>(data.get());
        std::fill(raw_ptr, raw_ptr + numPoints, value);
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
inline void RecordComponent::loadChunkRaw(T *ptr, Offset offset, Extent extent)
{
    prepareLoadStore()
        .offset(std::move(offset))
        .extent(std::move(extent))
        .withRawPtr(ptr)
        .load(EnqueuePolicy::Defer);
}

template <typename T>
inline void
RecordComponent::storeChunk(std::shared_ptr<T> data, Offset o, Extent e)
{
    prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .withSharedPtr(std::move(data))
        .enqueueStore();
}

template <typename T>
inline void
RecordComponent::storeChunk(UniquePtrWithLambda<T> data, Offset o, Extent e)
{
    prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .withUniquePtr(std::move(data))
        .enqueueStore();
}

template <typename T, typename Del>
inline void
RecordComponent::storeChunk(std::unique_ptr<T, Del> data, Offset o, Extent e)
{
    prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .withUniquePtr(std::move(data))
        .enqueueStore();
}

template <typename T>
void RecordComponent::storeChunkRaw(T *ptr, Offset offset, Extent extent)
{
    prepareLoadStore()
        .offset(std::move(offset))
        .extent(std::move(extent))
        .withRawPtr(ptr)
        .enqueueStore();
}

template <typename T_ContiguousContainer>
inline typename std::enable_if_t<
    auxiliary::IsContiguousContainer_v<T_ContiguousContainer>>
RecordComponent::storeChunk(T_ContiguousContainer &data, Offset o, Extent e)
{
    auto storeChunkConfig = prepareLoadStore();

    auto joined_dim = joinedDimension();
    if (!joined_dim.has_value() && (o.size() != 1 || o.at(0) != 0u))
    {
        storeChunkConfig.offset(std::move(o));
    }
    if (e.size() != 1 || e.at(0) != -1u)
    {
        storeChunkConfig.extent(std::move(e));
    }

    std::move(storeChunkConfig).withContiguousContainer(data).enqueueStore();
}

template <typename T, typename F>
inline DynamicMemoryView<T>
RecordComponent::storeChunk(Offset o, Extent e, F &&createBuffer)
{
    return prepareLoadStore()
        .offset(std::move(o))
        .extent(std::move(e))
        .enqueueStore<T>(std::forward<F>(createBuffer));
}

template <typename T>
inline DynamicMemoryView<T>
RecordComponent::storeChunk(Offset offset, Extent extent)
{
    return prepareLoadStore()
        .offset(std::move(offset))
        .extent(std::move(extent))
        .enqueueStore<T>();
}

template <typename T, typename F>
inline DynamicMemoryView<T> RecordComponent::storeChunkSpanCreateBuffer_impl(
    internal::LoadStoreConfig cfg, F &&createBuffer)
{
    auto [o, e] = std::move(cfg);
    verifyChunk<T>(o, e);

    /*
     * The openPMD backend might not yet know about this dataset.
     * Flush the openPMD hierarchy to the backend without flushing any actual
     * data yet.
     */
    seriesFlush({FlushLevel::SkeletonOnly});

    size_t size = 1;
    for (auto ext : e)
    {
        size *= ext;
    }
    /*
     * Flushing the skeleton does not create datasets,
     * so we might need to do it now.
     */
    if (!written())
    {
        auto &rc = get();
        Parameter<Operation::CREATE_DATASET> dCreate;
        dCreate.name = rc.m_name;
        dCreate.extent = getExtent();
        dCreate.dtype = getDatatype();
        dCreate.joinedDimension = joinedDimension();
        if (!rc.m_dataset.has_value())
        {
            throw error::WrongAPIUsage(
                "[RecordComponent] Must specify dataset type and extent before "
                "using storeChunk() (see RecordComponent::resetDataset()).");
        }
        dCreate.options = rc.m_dataset.value().options;
        IOHandler()->enqueue(IOTask(this, dCreate));
    }
    Parameter<Operation::GET_BUFFER_VIEW> getBufferView;
    getBufferView.offset = o;
    getBufferView.extent = e;
    getBufferView.dtype = getDatatype();
    IOHandler()->enqueue(IOTask(this, getBufferView));
    IOHandler()->flush(internal::defaultFlushParams);
    auto &out = *getBufferView.out;
    if (!out.backendManagedBuffer)
    {
        // note that data might have either
        // type shared_ptr<T> or shared_ptr<T[]>
        auto data = std::forward<F>(createBuffer)(size);
        out.ptr = static_cast<void *>(data.get());
        storeChunk(std::move(data), std::move(o), std::move(e));
    }
    setDirtyRecursive(true);
    return DynamicMemoryView<T>{std::move(getBufferView), size, *this};
}

namespace detail
{
    template <typename Functor, typename Res>
    struct VisitRecordComponent
    {
        template <typename T, typename... Args>
        static Res call(RecordComponent &rc, Args &&...args)
        {
            return Functor::template call<T>(rc, std::forward<Args>(args)...);
        }

        template <int = 0, typename... Args>
        static Res call(Args &&...)
        {
            throw std::runtime_error(
                "[RecordComponent::visit()] Unknown datatype in "
                "RecordComponent");
        }
    };
} // namespace detail

template <typename Visitor, typename... Args>
auto RecordComponent::visit(Args &&...args)
    -> decltype(Visitor::template call<char>(
        std::declval<RecordComponent &>(), std::forward<Args>(args)...))
{
    using Res = decltype(Visitor::template call<char>(
        std::declval<RecordComponent &>(), std::forward<Args>(args)...));
    return switchDatasetType<detail::VisitRecordComponent<Visitor, Res>>(
        getDatatype(), *this, std::forward<Args>(args)...);
}

template <typename T>
void RecordComponent::verifyChunk(Offset const &o, Extent const &e) const
{
    verifyChunk(determineDatatype<T>(), o, e);
}

// definitions for LoadStoreChunk.hpp
template <typename T, typename F>
auto core::ConfigureLoadStore::enqueueStore(F &&createBuffer)
    -> DynamicMemoryView<T>
{
    return m_rc.storeChunkSpanCreateBuffer_impl<T>(
        storeChunkConfig(), std::forward<F>(createBuffer));
}
} // namespace openPMD
