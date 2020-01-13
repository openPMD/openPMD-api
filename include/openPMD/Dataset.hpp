/* Copyright 2017-2020 Fabian Koller
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

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include "openPMD/Datatype.hpp"
#include <type_traits>


namespace openPMD
{
using Extent = std::vector< std::uint64_t >;
using Offset = std::vector< std::uint64_t >;

/**
 * @brief Restrict a block in a dataset to its intersection with another
 *        block.
 *
 * @param offset The offset wrt. the origin of the complete dataset of the block
 *               that is to be modified.
 * @param extent The extent of the mentioned block, wrt. to the block's own
 *               origin.
 * @param withinOffset The offset wrt. the origin of the complete dataset of the
 *                     second block that will not be modified.
 * @param withinExtent The extent of the mentioned block, wrt. to the block's
 *                     own origin.
 */
void
restrictToSelection(
    Offset & offset,
    Extent & extent,
    Offset const & withinOffset,
    Extent const & withinExtent );

/**
 * @brief Calculate the actual index of an element in an n-dimensional
 *        dataset stored in row-major fashion,
 *        given by n-dimensional coordinates.
 *
 * @param offset The n-dimensional index.
 * @param globalExtent The extent of the dataset in each dimension. If padding
 *                     is to be considered, the extent has to be given including
 *                     the padding.
 * @return size_t The index wrt. the first element of the dataset.
 */
size_t
rowMajorIndex( Offset const & offset, Extent const & globalExtent );

/**
 * @brief A chunk of actual data, tagged with meta information on the data's
 *        position in a dataset.
 *
 * @tparam T
 */
template< typename T >
struct TaggedChunk
{
    /**
     * @brief The offset of the data chunk with respect to the origin of the
     *        complete dataset.
     *
     */
    Offset offset;
    /**
     * @brief The extent of the data chunk with respect to its own origin.
     *
     */
    Extent extent;
    std::shared_ptr< T > data;

    TaggedChunk( Offset _offset, Extent _extent, std::shared_ptr< T > _data )
        : offset( std::move( _offset ) )
        , extent( std::move( _extent ) )
        , data( std::move( _data ) )
    {
    }

    void
    collectStrided( TaggedChunk< T > const & from );
};

struct RowMajorIterator
{
    Extent const extent;
    Offset current;
    size_t index = 0;
    RowMajorIterator( Extent );
    bool
    step();
};

class Dataset
{
    friend class RecordComponent;

public:
    Dataset(Datatype, Extent);

    Dataset& extend(Extent newExtent);
    Dataset& setChunkSize(Extent const&);
    Dataset& setCompression(std::string const&, uint8_t const);
    Dataset& setCustomTransform(std::string const&);

    Extent extent;
    Datatype dtype;
    uint8_t rank;
    Extent chunkSize;
    std::string compression;
    std::string transform;
};

template< typename T >
void
TaggedChunk< T >::collectStrided( TaggedChunk< T > const & from )
{
    // TODO check input
    Extent::value_type sliceLength = from.extent[ extent.size() - 1 ];
    size_t baseIndex;
    {
        Offset delta = from.offset;
        for( size_t i = 0; i < delta.size(); ++i )
        {
            delta[ i ] -= this->offset[ i ];
        }
        baseIndex = rowMajorIndex( delta, this->extent );
    }
    Extent prefix;
    prefix.reserve( from.extent.size() - 1 );
    for( size_t i = 0; i < from.extent.size() - 1; i++ )
    {
        prefix.push_back( from.extent[ i ] );
    }
    RowMajorIterator chunks( prefix );
    do
    {
        // from.offset + chunks.current is now equivalent to
        // the position of the slice in the global dataset
        // chunks.current is its position in from
        // from.offset - this->offset + chunks.current is its position in this
        // we have already calculated the base index
        // for from.offset - this.offset in the variable baseIndex
        chunks.current.push_back( 0 );
        size_t toIndex = rowMajorIndex( chunks.current, this->extent );
        chunks.current.pop_back();
        std::copy_n(
            from.data.get() + chunks.index * sliceLength,
            sliceLength,
            this->data.get() + baseIndex + toIndex );
    } while( chunks.step() );
}

} // openPMD
