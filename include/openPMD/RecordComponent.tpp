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

#include "openPMD/RecordComponent.hpp"
#include "openPMD/Span.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/ShareRawInternal.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

#include <memory>
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

    // default arguments
    //   offset = {0u}: expand to right dim {0u, 0u, ...}
    Offset offset = o;
    if (o.size() == 1u && o.at(0) == 0u && dim > 1u)
        offset = Offset(dim, 0u);

    //   extent = {-1u}: take full size
    Extent extent(dim, 1u);
    if (e.size() == 1u && e.at(0) == -1u)
    {
        extent = getExtent();
        for (uint8_t i = 0u; i < dim; ++i)
            extent[i] -= offset[i];
    }
    else
        extent = e;

    uint64_t numPoints = 1u;
    for (auto const &dimensionSize : extent)
        numPoints *= dimensionSize;

#if (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 11000) ||                   \
    (defined(__apple_build_version__) && __clang_major__ < 14)
    auto newData =
        std::shared_ptr<T>(new T[numPoints], [](T *p) { delete[] p; });
    loadChunk(newData, offset, extent);
    return newData;
#else
    auto newData = std::shared_ptr<T[]>(new T[numPoints]);
    loadChunk(newData, offset, extent);
    return std::static_pointer_cast<T>(std::move(newData));
#endif
}

template <typename T>
inline void
RecordComponent::loadChunk(std::shared_ptr<T> data, Offset o, Extent e)
{
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

    uint8_t dim = getDimensionality();

    // default arguments
    //   offset = {0u}: expand to right dim {0u, 0u, ...}
    Offset offset = o;
    if (o.size() == 1u && o.at(0) == 0u && dim > 1u)
        offset = Offset(dim, 0u);

    //   extent = {-1u}: take full size
    Extent extent(dim, 1u);
    if (e.size() == 1u && e.at(0) == -1u)
    {
        extent = getExtent();
        for (uint8_t i = 0u; i < dim; ++i)
            extent[i] -= offset[i];
    }
    else
        extent = e;

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
    if (!data)
        throw std::runtime_error(
            "Unallocated pointer passed during chunk loading.");

    auto &rc = get();
    if (constant())
    {
        uint64_t numPoints = 1u;
        for (auto const &dimensionSize : extent)
            numPoints *= dimensionSize;

        T value = rc.m_constantValue.get<T>();

        T *raw_ptr = data.get();
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
inline void RecordComponent::loadChunk(
    std::shared_ptr<T[]> ptr, Offset offset, Extent extent)
{
    loadChunk(
        std::static_pointer_cast<T>(std::move(ptr)),
        std::move(offset),
        std::move(extent));
}

template <typename T>
inline void RecordComponent::loadChunkRaw(T *ptr, Offset offset, Extent extent)
{
    loadChunk(auxiliary::shareRaw(ptr), std::move(offset), std::move(extent));
}

template <typename T>
inline void
RecordComponent::storeChunk(std::shared_ptr<T> data, Offset o, Extent e)
{
    if (!data)
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    Datatype dtype = determineDatatype(data);

    /* std::static_pointer_cast correctly reference-counts the pointer */
    storeChunk(
        auxiliary::WriteBuffer(std::static_pointer_cast<void const>(data)),
        dtype,
        std::move(o),
        std::move(e));
}

template <typename T>
inline void
RecordComponent::storeChunk(UniquePtrWithLambda<T> data, Offset o, Extent e)
{
    if (!data)
        throw std::runtime_error(
            "Unallocated pointer passed during chunk store.");
    Datatype dtype = determineDatatype<>(data);

    storeChunk(
        auxiliary::WriteBuffer{std::move(data).template static_cast_<void>()},
        dtype,
        std::move(o),
        std::move(e));
}

template <typename T, typename Del>
inline void
RecordComponent::storeChunk(std::unique_ptr<T, Del> data, Offset o, Extent e)
{
    storeChunk(
        UniquePtrWithLambda<T>(std::move(data)), std::move(o), std::move(e));
}

template <typename T>
inline void
RecordComponent::storeChunk(std::shared_ptr<T[]> data, Offset o, Extent e)
{
    storeChunk(
        std::static_pointer_cast<T>(std::move(data)),
        std::move(o),
        std::move(e));
}

template <typename T>
void RecordComponent::storeChunkRaw(T *ptr, Offset offset, Extent extent)
{
    storeChunk(auxiliary::shareRaw(ptr), std::move(offset), std::move(extent));
}

template <typename T_ContiguousContainer>
inline typename std::enable_if_t<
    auxiliary::IsContiguousContainer_v<T_ContiguousContainer>>
RecordComponent::storeChunk(T_ContiguousContainer &data, Offset o, Extent e)
{
    uint8_t dim = getDimensionality();

    // default arguments
    //   offset = {0u}: expand to right dim {0u, 0u, ...}
    Offset offset = o;
    if (o.size() == 1u && o.at(0) == 0u)
    {
        if (joinedDimension().has_value())
        {
            offset.clear();
        }
        else if (dim > 1u)
        {
            offset = Offset(dim, 0u);
        }
    }

    //   extent = {-1u}: take full size
    Extent extent(dim, 1u);
    //   avoid outsmarting the user:
    //   - stdlib data container implement 1D -> 1D chunk to write
    if (e.size() == 1u && e.at(0) == -1u && dim == 1u)
        extent.at(0) = data.size();
    else
        extent = e;

    storeChunk(auxiliary::shareRaw(data.data()), offset, extent);
}

template <typename T, typename F>
inline DynamicMemoryView<T>
RecordComponent::storeChunk(Offset o, Extent e, F &&createBuffer)
{
    verifyChunk<T>(o, e);

    /*
     * The openPMD backend might not yet know about this dataset.
     * Flush the openPMD hierarchy to the backend without flushing any actual
     * data yet.
     */
    seriesFlush_impl</* flush_entire_series = */ false>(
        {FlushLevel::SkeletonOnly});

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

template <typename T>
inline DynamicMemoryView<T>
RecordComponent::storeChunk(Offset offset, Extent extent)
{
    return storeChunk<T>(std::move(offset), std::move(extent), [](size_t size) {
#if (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 11000) ||                   \
    (defined(__apple_build_version__) && __clang_major__ < 14)
        return std::shared_ptr<T>{new T[size], [](auto *ptr) { delete[] ptr; }};
#else
            return std::shared_ptr< T[] >{ new T[ size ] };
#endif
    });
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
} // namespace openPMD
