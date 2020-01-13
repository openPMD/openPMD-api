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
#include "openPMD/Dataset.hpp"

#include <algorithm>
#include <cstddef>
#include <iostream>


namespace openPMD
{
void
restrictToSelection(
    Offset & offset,
    Extent & extent,
    Offset const & withinOffset,
    Extent const & withinExtent )
{
    for( size_t i = 0; i < offset.size(); ++i )
    {
        if( offset[ i ] < withinOffset[ i ] )
        {
            auto delta = withinOffset[ i ] - offset[ i ];
            offset[ i ] = withinOffset[ i ];
            if( delta > extent[ i ] )
            {
                extent[ i ] = 0;
            }
            else
            {
                extent[ i ] -= delta;
            }
        }
        auto totalExtent = extent[ i ] + offset[ i ];
        auto totalWithinExtent = withinExtent[ i ] + withinOffset[ i ];
        if( totalExtent > totalWithinExtent )
        {
            auto delta = totalExtent - totalWithinExtent;
            if( delta > extent[ i ] )
            {
                extent[ i ] = 0;
            }
            else
            {
                extent[ i ] -= delta;
            }
        }
    }
}

size_t
rowMajorIndex( Offset const & offset, Extent const & globalExtent )
{
    size_t res = 0;
    size_t factor = 1;
    size_t i = offset.size();
    do
    {
        --i;
        res += offset[ i ] * factor;
        factor *= globalExtent[ i ];
    } while( i != 0 );
    return res;
}

RowMajorIterator::RowMajorIterator( Extent _extent )
    : extent( _extent ), current( extent.size(), 0 )
{
}

bool
RowMajorIterator::step()
{
    size_t dim = extent.size();
    while( dim > 0 )
    {
        --dim;
        if( current[ dim ] + 1 >= extent[ dim ] )
        {
            current[ dim ] = 0;
            continue;
        }
        ++current[ dim ];
        ++index;
        return true;
    }
    return false;
}

Dataset::Dataset( Datatype d, Extent e )
    : extent{ e }
    , dtype{ d }
    , rank{ static_cast< uint8_t >( e.size() ) }
    , chunkSize{ e }
{ }

Dataset&
Dataset::extend(Extent newExtents)
{
    if( newExtents.size() != rank )
        throw std::runtime_error("Dimensionality of extended Dataset must match the original dimensionality");
    for( size_t i = 0; i < newExtents.size(); ++i )
        if( newExtents[i] < extent[i] )
            throw std::runtime_error("New Extent must be equal or greater than previous Extent");

    extent = newExtents;
    return *this;
}

Dataset&
Dataset::setChunkSize(Extent const& cs)
{
    if( extent.size() != rank )
        throw std::runtime_error("Dimensionality of extended Dataset must match the original dimensionality");
    for( size_t i = 0; i < cs.size(); ++i )
        if( cs[i] > extent[i] )
            throw std::runtime_error("Dataset chunk size must be equal or smaller than Extent");

    chunkSize = cs;
    return *this;
}

Dataset&
Dataset::setCompression(std::string const& format, uint8_t const level)
{
    if(format == "zlib" || format == "gzip" || format == "deflate")
    {
        if(level > 9)
            throw std::runtime_error("Compression level out of range for " + format);
    }
    else
        std::cerr << "Unknown compression format " << format
                  << ". This might mean that compression will not be enabled."
                  << std::endl;

    compression = format + ':' + std::to_string(static_cast< int >(level));
    return *this;
}

Dataset&
Dataset::setCustomTransform(std::string const& parameter)
{
    transform = parameter;
    return *this;
}
} // namespace openPMD
