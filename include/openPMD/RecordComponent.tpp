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

namespace openPMD
{
template< typename T >
inline RecordComponent&
RecordComponent::makeConstant(T value)
{
    if( written() )
        throw std::runtime_error("A recordComponent can not (yet) be made constant after it has been written.");

    *m_constantValue = Attribute(value);
    *m_isConstant = true;
    return *this;
}

template< typename T >
inline RecordComponent&
RecordComponent::makeEmpty( uint8_t dimensions )
{
    return makeEmpty( Dataset(
        determineDatatype< T >(),
        Extent( dimensions, 0 ) ) );
}

template< typename T >
inline std::shared_ptr< T > RecordComponent::loadChunk(
    Offset o, Extent e )
{
    uint8_t dim = getDimensionality();

    // default arguments
    //   offset = {0u}: expand to right dim {0u, 0u, ...}
    Offset offset = o;
    if( o.size() == 1u && o.at(0) == 0u && dim > 1u )
        offset = Offset(dim, 0u);

    //   extent = {-1u}: take full size
    Extent extent(dim, 1u);
    if( e.size() == 1u && e.at(0) == -1u )
    {
        extent = getExtent();
        for( uint8_t i = 0u; i < dim; ++i )
            extent[i] -= offset[i];
    }
    else
        extent = e;

    uint64_t numPoints = 1u;
    for( auto const& dimensionSize : extent )
        numPoints *= dimensionSize;

    auto newData = std::shared_ptr<T>(new T[numPoints], []( T *p ){ delete [] p; });
    loadChunk(newData, offset, extent);
    return newData;
}

template< typename T >
inline void RecordComponent::loadChunk(
    std::shared_ptr< T > data,
    Offset o,
    Extent e )
{
    Datatype dtype = determineDatatype(data);
    if( dtype != getDatatype() )
        if( !isSameInteger< T >( getDatatype() ) &&
            !isSameFloatingPoint< T >( getDatatype() ) &&
            !isSameComplexFloatingPoint< T >( getDatatype() ) )
            throw std::runtime_error(
                "Type conversion during chunk loading not yet implemented" );

    uint8_t dim = getDimensionality();

    // default arguments
    //   offset = {0u}: expand to right dim {0u, 0u, ...}
    Offset offset = o;
    if( o.size() == 1u && o.at(0) == 0u && dim > 1u )
        offset = Offset(dim, 0u);

    //   extent = {-1u}: take full size
    Extent extent(dim, 1u);
    if( e.size() == 1u && e.at(0) == -1u )
    {
        extent = getExtent();
        for( uint8_t i = 0u; i < dim; ++i )
            extent[i] -= offset[i];
    }
    else
        extent = e;

    if( extent.size() != dim || offset.size() != dim )
    {
        std::ostringstream oss;
        oss << "Dimensionality of chunk ("
            << "offset=" << offset.size() << "D, "
            << "extent=" << extent.size() << "D) "
            << "and record component ("
            << int(dim) << "D) "
            << "do not match.";
        throw std::runtime_error(oss.str());
    }
    Extent dse = getExtent();
    for( uint8_t i = 0; i < dim; ++i )
        if( dse[i] < offset[i] + extent[i] )
            throw std::runtime_error("Chunk does not reside inside dataset (Dimension on index " + std::to_string(i)
                                     + ". DS: " + std::to_string(dse[i])
                                     + " - Chunk: " + std::to_string(offset[i] + extent[i])
                                     + ")");
    if( !data )
        throw std::runtime_error("Unallocated pointer passed during chunk loading.");

    if( constant() )
    {
        uint64_t numPoints = 1u;
        for( auto const& dimensionSize : extent )
            numPoints *= dimensionSize;

        T value = m_constantValue->get< T >();

        T* raw_ptr = data.get();
        std::fill(raw_ptr, raw_ptr + numPoints, value);
    } else
    {
        Parameter< Operation::READ_DATASET > dRead;
        dRead.offset = offset;
        dRead.extent = extent;
        dRead.dtype = getDatatype();
        dRead.data = std::static_pointer_cast< void >(data);
        m_chunks->push(IOTask(this, dRead));
    }
}

template< typename T >
inline void
RecordComponent::storeChunk(std::shared_ptr<T> data, Offset o, Extent e)
{
    if( constant() )
        throw std::runtime_error("Chunks cannot be written for a constant RecordComponent.");
    if( empty() )
        throw std::runtime_error("Chunks cannot be written for an empty RecordComponent.");
    if( !data )
        throw std::runtime_error("Unallocated pointer passed during chunk store.");
    Datatype dtype = determineDatatype(data);
    if( dtype != getDatatype() )
    {
        std::ostringstream oss;
        oss << "Datatypes of chunk data ("
            << dtype
            << ") and record component ("
            << getDatatype()
            << ") do not match.";
        throw std::runtime_error(oss.str());
    }
    uint8_t dim = getDimensionality();
    if( e.size() != dim || o.size() != dim )
    {
        std::ostringstream oss;
        oss << "Dimensionality of chunk ("
            << "offset=" << o.size() << "D, "
            << "extent=" << e.size() << "D) "
            << "and record component ("
            << int(dim) << "D) "
            << "do not match.";
        throw std::runtime_error(oss.str());
    }
    Extent dse = getExtent();
    for( uint8_t i = 0; i < dim; ++i )
        if( dse[i] < o[i] + e[i] )
            throw std::runtime_error("Chunk does not reside inside dataset (Dimension on index " + std::to_string(i)
                                     + ". DS: " + std::to_string(dse[i])
                                     + " - Chunk: " + std::to_string(o[i] + e[i])
                                     + ")");

    Parameter< Operation::WRITE_DATASET > dWrite;
    dWrite.offset = o;
    dWrite.extent = e;
    dWrite.dtype = dtype;
    /* std::static_pointer_cast correctly reference-counts the pointer */
    dWrite.data = std::static_pointer_cast< void const >(data);
    m_chunks->push(IOTask(this, dWrite));
}

template< typename T_ContiguousContainer >
inline typename std::enable_if<
    traits::IsContiguousContainer< T_ContiguousContainer >::value
>::type
RecordComponent::storeChunk(T_ContiguousContainer & data, Offset o, Extent e)
{
    uint8_t dim = getDimensionality();

    // default arguments
    //   offset = {0u}: expand to right dim {0u, 0u, ...}
    Offset offset = o;
    if( o.size() == 1u && o.at(0) == 0u && dim > 1u )
        offset = Offset(dim, 0u);

    //   extent = {-1u}: take full size
    Extent extent(dim, 1u);
    //   avoid outsmarting the user:
    //   - stdlib data container implement 1D -> 1D chunk to write
    if( e.size() == 1u && e.at(0) == -1u && dim == 1u )
        extent.at(0) = data.size();
    else
        extent = e;

    storeChunk(shareRaw(data), offset, extent);
}

template< typename T, typename F >
inline DynamicMemoryView< T >
RecordComponent::storeChunk( Offset o, Extent e, F && createBuffer )
{
    if( constant() )
        throw std::runtime_error(
            "Chunks cannot be written for a constant RecordComponent." );
    if( empty() )
        throw std::runtime_error(
            "Chunks cannot be written for an empty RecordComponent." );
    Datatype dtype = determineDatatype<T>();
    if( dtype != getDatatype() )
    {
        std::ostringstream oss;
        oss << "Datatypes of chunk data ("
            << dtype
            << ") and record component ("
            << getDatatype()
            << ") do not match.";
        throw std::runtime_error(oss.str());
    }
    uint8_t dim = getDimensionality();
    if( e.size() != dim || o.size() != dim )
    {
        std::ostringstream oss;
        oss << "Dimensionality of chunk ("
            << "offset=" << o.size() << "D, "
            << "extent=" << e.size() << "D) "
            << "and record component ("
            << int(dim) << "D) "
            << "do not match.";
        throw std::runtime_error(oss.str());
    }
    Extent dse = getExtent();
    for( uint8_t i = 0; i < dim; ++i )
        if( dse[i] < o[i] + e[i] )
            throw std::runtime_error("Chunk does not reside inside dataset (Dimension on index " + std::to_string(i)
                                     + ". DS: " + std::to_string(dse[i])
                                     + " - Chunk: " + std::to_string(o[i] + e[i])
                                     + ")");

    /*
     * The openPMD backend might not yet know about this dataset.
     * Flush the openPMD hierarchy to the backend without flushing any actual
     * data yet.
     */
    seriesFlush({FlushLevel::SkeletonOnly});

    size_t size = 1;
    for( auto ext : e )
    {
        size *= ext;
    }
    /*
     * Flushing the skeleton does not create datasets,
     * so we might need to do it now.
     */
    if( !written() )
    {
        Parameter< Operation::CREATE_DATASET > dCreate;
        dCreate.name = *m_name;
        dCreate.extent = getExtent();
        dCreate.dtype = getDatatype();
        dCreate.chunkSize = m_dataset->chunkSize;
        dCreate.compression = m_dataset->compression;
        dCreate.transform = m_dataset->transform;
        dCreate.options = m_dataset->options;
        IOHandler()->enqueue(IOTask(this, dCreate));
    }
    Parameter< Operation::GET_BUFFER_VIEW > getBufferView;
    getBufferView.offset = o;
    getBufferView.extent = e;
    getBufferView.dtype = getDatatype();
    IOHandler()->enqueue(IOTask(this, getBufferView));
    IOHandler()->flush(internal::defaultFlushParams);
    auto &out = *getBufferView.out;
    if (!out.backendManagedBuffer)
    {
        auto data = std::forward<F>(createBuffer)(size);
        out.ptr = static_cast<void *>(data.get());
        storeChunk(std::move(data), std::move(o), std::move(e));
    }
    return DynamicMemoryView<T>{std::move(getBufferView), size, *this};
}

template< typename T >
inline DynamicMemoryView< T >
RecordComponent::storeChunk( Offset offset, Extent extent )
{
    return storeChunk< T >(
        std::move( offset ),
        std::move( extent ),
        []( size_t size )
        {
            return std::shared_ptr< T >{
                new T[ size ], []( auto * ptr ) { delete[] ptr; } };
        } );
}
}
