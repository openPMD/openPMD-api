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
#include <stdexcept>
#include <vector>
#include <array>


namespace openPMD
{
//! @{
/** Share ownership with a raw pointer
 *
 * Helper function to share write ownership
 * unprotected and without reference counting with a
 * raw pointer or stdlib container.
 *
 * @warning this is a helper function to bypass the shared-pointer
 *          API for storing data behind raw pointers. Using it puts
 *          the resposibility of buffer-consistency between stores
 *          and flushes to the users side without an indication via
 *          reference counting.
 */
template< typename T >
std::shared_ptr< T >
storeRaw( T* x )
{
    return std::shared_ptr< T >( x, [](T*){} );
}

template< typename T >
std::shared_ptr< T >
storeRaw( std::vector< T > & v )
{
    return std::shared_ptr< T >( v.data(), [](T*){} );
}

template< typename T, std::size_t T_size >
std::shared_ptr< T >
storeRaw( std::array< T, T_size > & a )
{
    return std::shared_ptr< T >( a.data(), [](T*){} );
}
//! @}

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
    };  //Allocation

    RecordComponent& setUnitSI(double);

    RecordComponent& resetDataset(Dataset);

    uint8_t getDimensionality() const;
    Extent getExtent() const;

    template< typename T >
    RecordComponent& makeConstant(T);

    template< typename T, typename D >
    void loadChunk(Offset const&,
                   Extent const&,
                   std::unique_ptr< T[], D >&,
                   Allocation = Allocation::AUTO,
                   double targetUnitSI = std::numeric_limits< double >::quiet_NaN() );
    template< typename T >
    void storeChunk(Offset, Extent, std::shared_ptr< T >);

    constexpr static char const * const SCALAR = "\vScalar";

protected:
    RecordComponent();

    void readBase();

    std::shared_ptr< std::queue< IOTask > > m_chunks;
    std::shared_ptr< Attribute > m_constantValue;

private:
    void flush(std::string const&);
    virtual void read();
};  //RecordComponent


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

template< typename T, typename D >
inline void
RecordComponent::loadChunk(Offset const& o, Extent const& e, std::unique_ptr< T[], D >& data, Allocation alloc, double targetUnitSI)
{
    if( !std::isnan(targetUnitSI) )
        throw std::runtime_error("unitSI scaling during chunk loading not yet implemented");
    Datatype dtype = determineDatatype(std::shared_ptr< T >());
    if( dtype != getDatatype() )
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
    if( Allocation::API == alloc && data )
        throw std::runtime_error("Preallocated pointer passed with signaled API-allocation during chunk loading.");
    else if( Allocation::USER == alloc && !data )
        throw std::runtime_error("Unallocated pointer passed with signaled user-allocation during chunk loading.");

    uint64_t numPoints = 1u;
    for( auto const& dimensionSize : e )
        numPoints *= dimensionSize;

    if( (Allocation::AUTO == alloc && !data) || Allocation::API == alloc )
        data = std::unique_ptr< T[], D >(new T[numPoints]);
    T* raw_ptr = data.get();

    if( *m_isConstant )
    {
        /*
        Parameter< Operation::READ_ATT > aRead;
        aRead.name = "value";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();
        T value = Attribute(*aRead.resource).get< T >();
         */
        T value = m_constantValue->get< T >();
        std::fill(raw_ptr, raw_ptr + numPoints, value);
    } else
    {
        Parameter< Operation::READ_DATASET > dRead;
        dRead.offset = o;
        dRead.extent = e;
        dRead.dtype = getDatatype();
        dRead.data = raw_ptr;
        IOHandler->enqueue(IOTask(this, dRead));
        IOHandler->flush();
    }
}

//template< typename T >
//inline std::unique_ptr< T, std::function< void(T*) > >
//RecordComponent::loadChunk(Offset o, Extent e, double targetUnitSI)
//{
//    if( targetUnitSI != 0. )
//        throw std::runtime_error("unitSI scaling during chunk loading not yet implemented");
//    Datatype dtype = determineDatatype(std::shared_ptr< T >());
//    if( dtype != getDatatype() )
//        throw std::runtime_error("Type conversion during chunk loading not implemented yet");
//    uint8_t dim = getDimensionality();
//    if( e.size() != dim || o.size() != dim )
//        throw std::runtime_error("Dimensionality of chunk and dataset do not match.");
//    Extent dse = getExtent();
//    for( uint8_t i = 0; i < dim; ++i )
//        if( dse[i] < o[i] + e[i] )
//            throw std::runtime_error("Chunk does not reside inside dataset (Dimension on index " + std::to_string(i)
//                                     + " - DS: " + std::to_string(dse[i])
//                                     + " - Chunk: " + std::to_string(o[i] + e[i])
//                                     + ")");
//
//    uint64_t numPoints = 1u;
//    for( auto const& dimensionSize : e )
//        numPoints *= dimensionSize;
//
//    auto data = std::move(allocatePtr(getDatatype(), numPoints));
//    void *ptr = data.get();
//
//    if( *m_isConstant )
//    {
//        Parameter< Operation::READ_ATT > attribute_parameter;
//        attribute_parameter.name = "value";
//        IOHandler->enqueue(IOTask(this, attribute_parameter));
//        IOHandler->flush();
//        T* ptr = static_cast< T* >(data);
//        T value = Attribute(*attribute_parameter.resource).get< T >();
//        std::fill(ptr, ptr + numPoints, value);
//    } else
//    {
//        Parameter< Operation::READ_DATASET > chunk_parameter;
//        chunk_parameter.offset = o;
//        chunk_parameter.extent = e;
//        chunk_parameter.dtype = getDatatype();
//        chunk_parameter.data = data;
//        IOHandler->enqueue(IOTask(this, chunk_parameter));
//        IOHandler->flush();
//    }
//
//    T* ptr = static_cast< T* >(data);
//    auto deleter = [](T* p){ delete[] p; p = nullptr; };
//    return std::unique_ptr< T, decltype(deleter) >(ptr, deleter);
//}

template< typename T >
inline void
RecordComponent::storeChunk(Offset o, Extent e, std::shared_ptr<T> data)
{
    if( *m_isConstant )
        throw std::runtime_error("Chunks can not be written for a constant RecordComponent.");
    Datatype dtype = determineDatatype(data);
    if( dtype != getDatatype() )
        throw std::runtime_error("Datatypes of chunk and dataset do not match.");
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
    dWrite.data = std::static_pointer_cast< void >(data);
    m_chunks->push(IOTask(this, dWrite));
}
} // openPMD
