/* Copyright 2017-2018 Fabian Koller
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

#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/Dataset.hpp"

#include <cmath>
#include <memory>
#include <limits>
#include <queue>
#include <string>
#include <sstream>
#include <stdexcept>

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#   define OPENPMD_protected protected
#endif


namespace openPMD
{
class RecordComponent : public BaseRecordComponent
{
    template<
            typename T,
            typename T_key,
            typename T_container
    >
    friend class Container;
    friend class Iteration;
    friend class ParticleSpecies;
    template< typename T_elem >
    friend class BaseRecord;
    friend class Record;
    friend class Mesh;

public:
    enum class Allocation
    {
        USER,
        API,
        AUTO
    }; // Allocation

    RecordComponent& setUnitSI(double);

    RecordComponent& resetDataset(Dataset);

    uint8_t getDimensionality() const;
    Extent getExtent() const;

    template< typename T >
    RecordComponent& makeConstant(T);

    template< typename T >
    std::shared_ptr< T > loadChunk(Offset const&,
                   Extent const&,
                   double targetUnitSI = std::numeric_limits< double >::quiet_NaN() );

    template< typename T >
    void loadChunk(Offset const&,
                   Extent const&,
                   std::shared_ptr< T >,
                   double targetUnitSI = std::numeric_limits< double >::quiet_NaN() );
    template< typename T >
    void storeChunk(Offset, Extent, std::shared_ptr< T >);

    constexpr static char const * const SCALAR = "\vScalar";

OPENPMD_protected:
    RecordComponent();

    void readBase();

    std::shared_ptr< std::queue< IOTask > > m_chunks;
    std::shared_ptr< Attribute > m_constantValue;

private:
    void flush(std::string const&);
    virtual void read();
}; // RecordComponent


template< typename T >
inline RecordComponent&
RecordComponent::makeConstant(T value)
{
    if( written )
        throw std::runtime_error("A recordComponent can not (yet) be made constant after it has been written.");

    *m_constantValue = Attribute(value);
    *m_isConstant = true;
    return *this;
}

template< typename T >
inline std::shared_ptr< T >
RecordComponent::loadChunk(Offset const& o, Extent const& e, double targetUnitSI)
{
    uint64_t numPoints = 1u;
    for( auto const& dimensionSize : e )
        numPoints *= dimensionSize;

    auto newData = std::shared_ptr<T>(new T[numPoints], []( T *p ){ delete [] p; });
    loadChunk(o, e, newData, targetUnitSI);
    return newData;
}

template< typename T >
inline void
RecordComponent::loadChunk(Offset const& o, Extent const& e, std::shared_ptr< T > data, double targetUnitSI)
{
    if( !std::isnan(targetUnitSI) )
        throw std::runtime_error("unitSI scaling during chunk loading not yet implemented");
    Datatype dtype = determineDatatype(data);
    if( dtype != getDatatype() )
        if( !isSameInteger< T >( dtype ) && !isSameFloatingPoint< T >( dtype ) )
            throw std::runtime_error("Type conversion during chunk loading not yet implemented");

    uint8_t dim = getDimensionality();
    if( e.size() != dim || o.size() != dim )
        throw std::runtime_error("Dimensionality of chunk and dataset do not match.");
    Extent dse = getExtent();
    for( uint8_t i = 0; i < dim; ++i )
        if( dse[i] < o[i] + e[i] )
            throw std::runtime_error("Chunk does not reside inside dataset (Dimension on index " + std::to_string(i)
                                     + " - DS: " + std::to_string(dse[i])
                                     + " - Chunk: " + std::to_string(o[i] + e[i])
                                     + ")");
    if( !data )
        throw std::runtime_error("Unallocated pointer passed during chunk loading.");

    if( *m_isConstant )
    {
        uint64_t numPoints = 1u;
        for( auto const& dimensionSize : e )
            numPoints *= dimensionSize;

        T value = m_constantValue->get< T >();

        T* raw_ptr = data.get();
        std::fill(raw_ptr, raw_ptr + numPoints, value);
    } else
    {
        Parameter< Operation::READ_DATASET > dRead;
        dRead.offset = o;
        dRead.extent = e;
        dRead.dtype = getDatatype();
        dRead.data = std::static_pointer_cast< void >(data);
        m_chunks->push(IOTask(this, dRead));
    }
}

template< typename T >
inline void
RecordComponent::storeChunk(Offset o, Extent e, std::shared_ptr<T> data)
{
    if( *m_isConstant )
        throw std::runtime_error("Chunks can not be written for a constant RecordComponent.");
    if( !data )
        throw std::runtime_error("Unallocated pointer passed during chunk store.");
    Datatype dtype = determineDatatype(data);
    if( dtype != getDatatype() )
    {
        std::ostringstream oss;
        oss << "Datatypes of chunk data ("
            << dtype
            << ") and dataset ("
            << getDatatype()
            << ") do not match.";
        throw std::runtime_error(oss.str());
    }
    uint8_t dim = getDimensionality();
    if( e.size() != dim || o.size() != dim )
        throw std::runtime_error("Dimensionality of chunk and dataset do not match.");
    Extent dse = getExtent();
    for( uint8_t i = 0; i < dim; ++i )
        if( dse[i] < o[i] + e[i] )
            throw std::runtime_error("Chunk does not reside inside dataset (Dimension on index " + std::to_string(i)
                                     + " - DS: " + std::to_string(dse[i])
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
} // namespace openPMD
