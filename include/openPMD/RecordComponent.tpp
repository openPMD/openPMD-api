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

#include "openPMD/Datatype.hpp"
#include "openPMD/Error.hpp"
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

template <typename T, typename Del>
inline void
RecordComponent::storeChunk(std::unique_ptr<T, Del> data, Offset o, Extent e)
{
    storeChunk(
        UniquePtrWithLambda<T>(std::move(data)), std::move(o), std::move(e));
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
        if (!rc.m_dataset.has_value())
        {
            throw error::WrongAPIUsage(
                "[RecordComponent] Must specify dataset type and extent before "
                "using storeChunk() (see RecordComponent::resetDataset()).");
        }
        Parameter<Operation::CREATE_DATASET> dCreate(rc.m_dataset.value());
        dCreate.name = rc.m_name;
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
inline auto RecordComponent::visit(Args &&...args)
    -> decltype(Visitor::template call<char>(
        std::declval<RecordComponent &>(), std::forward<Args>(args)...))
{
    using Res = decltype(Visitor::template call<char>(
        std::declval<RecordComponent &>(), std::forward<Args>(args)...));
    return switchDatasetType<detail::VisitRecordComponent<Visitor, Res>>(
        getDatatype(), *this, std::forward<Args>(args)...);
}
} // namespace openPMD
