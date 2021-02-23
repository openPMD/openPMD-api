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
#pragma once

#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/auxiliary/ShareRaw.hpp"
#include "openPMD/Dataset.hpp"

#include <cmath>
#include <memory>
#include <limits>
#include <queue>
#include <string>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <array>

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#   define OPENPMD_protected protected
#endif


namespace openPMD
{
namespace traits
{
/** Emulate in the C++17 concept ContiguousContainer
 *
 * Users can implement this trait for a type to signal it can be used as
 * contiguous container.
 *
 * See:
 *   https://en.cppreference.com/w/cpp/named_req/ContiguousContainer
 */
template< typename T >
struct IsContiguousContainer
{
    static constexpr bool value = false;
};

template< typename T_Value >
struct IsContiguousContainer< std::vector< T_Value > >
{
    static constexpr bool value = true;
};

template<
    typename T_Value,
    std::size_t N
>
struct IsContiguousContainer< std::array< T_Value, N > >
{
    static constexpr bool value = true;
};
} // namespace traits

/**
 * @brief Subset of C++20 std::span class template.
 *
 * Any existing member behaves equivalently to those documented here:
 * https://en.cppreference.com/w/cpp/container/span
 */
template< typename T >
class Span
{
    friend class RecordComponent;

private:
    using param_t = Parameter< Operation::GET_BUFFER_VIEW >;
    param_t m_param;
    size_t m_size;
    // @todo make this safe
    Writable * m_writable;

    Span( param_t param, size_t size, Writable * writable ) :
        m_param( std::move( param ) ),
        m_size( size ),
        m_writable( std::move( writable ) )
    {
        m_param.update = true;
    }

public:
    size_t size() const
    {
        return m_size;
    }

    T *data() const
    {
        if( m_param.out->taskSupportedByBackend )
        {
            // might need to update
            m_writable->IOHandler->enqueue( IOTask( m_writable, m_param ) );
            m_writable->IOHandler->flush();
        }
        return static_cast< T * >( m_param.out->ptr );
    }

    T &operator[]( size_t i ) const
    {
        return data()[ i ];
    }
};

template class Span< int >;

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
    template< typename >
    friend class Span;

public:
    enum class Allocation
    {
        USER,
        API,
        AUTO
    }; // Allocation

    RecordComponent& setUnitSI(double);

    /**
     * @brief Declare the dataset's type and extent.
     *
     * Calling this again after flushing will require resizing the dataset.
     * Support for this depends on the backend.
     * Unsupported are:
     * * Changing the datatype.
     * * Shrinking any dimension's extent.
     * * Changing the number of dimensions.
     *
     * Backend support for resizing datasets:
     * * JSON: Supported
     * * ADIOS1: Unsupported
     * * ADIOS2: Supported as of ADIOS2 2.7.0
     * * HDF5: (Currently) unsupported.
     *   Will be probably supported as soon as chunking is supported in HDF5.
     *
     * @return RecordComponent&
     */
    RecordComponent & resetDataset( Dataset );

    uint8_t getDimensionality() const;
    Extent getExtent() const;

    /** Create a dataset with regular extent and constant value
     *
     * In a constant record component, the value for each date in its extent is
     * the same. Implemented by storing only a constant value as meta-data.
     *
     * @tparam T type of the stored value
     * @return A reference to this RecordComponent.
     */
    template< typename T >
    RecordComponent& makeConstant(T);

    /** Create a dataset with zero extent in each dimension.
     *
     * Implemented by creating a constant record component with
     * a dummy value (default constructor of the respective datatype).
     * @param dimensions The number of dimensions. Must be greater than
     *                   zero.
     * @return A reference to this RecordComponent.
     */
    template< typename T >
    RecordComponent& makeEmpty( uint8_t dimensions );

    /**
     * @brief Non-template overload of RecordComponent::makeEmpty().
     * Uses the passed openPMD datatype to determine the template parameter.
     *
     * @param dt The datatype of which to create an empty dataset.
     * @param dimensions The dimensionality of the dataset.
     * @return RecordComponent&
     */
    RecordComponent& makeEmpty( Datatype dt, uint8_t dimensions );

    /** Returns true if this is an empty record component
     *
     * An empty record component has a defined dimensionality but zero extent
     * and no value(s) stored in it.
     *
     * @return true if an empty record component
     */
    bool empty() const;

    /** Load and allocate a chunk of data
     *
     * Set offset to {0u} and extent to {-1u} for full selection.
     *
     * If offset is non-zero and extent is {-1u} the leftover extent in the
     * record component will be selected.
     */
    template< typename T >
    std::shared_ptr< T > loadChunk(
        Offset = { 0u },
        Extent = { -1u } );

    /** Load a chunk of data into pre-allocated memory
     *
     * shared_ptr for data must be pre-allocated, contiguous and large enough for extent
     *
     * Set offset to {0u} and extent to {-1u} for full selection.
     *
     * If offset is non-zero and extent is {-1u} the leftover extent in the
     * record component will be selected.
     */
    template< typename T >
    void loadChunk(
        std::shared_ptr< T >,
        Offset,
        Extent );

    template< typename T >
    void storeChunk(std::shared_ptr< T >, Offset, Extent);

    template< typename T_ContiguousContainer >
    typename std::enable_if<
        traits::IsContiguousContainer< T_ContiguousContainer >::value
    >::type
    storeChunk(T_ContiguousContainer &, Offset = {0u}, Extent = {-1u});

    /**
     * @brief Overload of storeChunk() that lets the openPMD API allocate
     *        a buffer.
     *
     * This may save memory if the openPMD backend in use is able to provide
     * users a view into its own buffers, avoiding the need to allocate
     * a new buffer.
     *
     * Data can be written into the returned buffer until the next call to
     * Series::flush() at which time the data will be read from.
     *
     * In order to provide a view into backend buffers, this call must possibly
     * create files and datasets in the backend, making it MPI-collective.
     * In order to avoid this, calling Series::flush() prior to this is
     * recommended to flush definitions.
     *
     * @param createBuffer If the backend in use as no special support for this
     *        operation, the openPMD API will fall back to creating a buffer,
     *        queuing it for writing and returning a view into that buffer to
     *        the user. The functor createBuffer will be called for this
     *        purpose. It consumes a length parameter of type size_t and should
     *        return a shared_ptr of type T to a buffer at least that length.
     *
     * @return View into a buffer that can be filled with data.
     */
    template< typename T, typename F >
    Span< T > storeChunk( Offset, Extent, F && createBuffer );

    /**
     * Overload of span-based storeChunk() that uses operator new() to create
     * a buffer.
     */
    template< typename T >
    Span< T > storeChunk( Offset, Extent );

    static constexpr char const * const SCALAR = "\vScalar";

    virtual ~RecordComponent() = default;

OPENPMD_protected:
    RecordComponent();

    void readBase();

    std::shared_ptr< std::queue< IOTask > > m_chunks;
    std::shared_ptr< Attribute > m_constantValue;
    std::shared_ptr< bool > m_isEmpty = std::make_shared< bool >( false );
    // User has extended the dataset, but the EXTEND task must yet be flushed
    // to the backend
    std::shared_ptr< bool > m_hasBeenExtended =
        std::make_shared< bool >( false );

private:
    void flush(std::string const&);
    virtual void read();

    /**
     * Internal method to be called by all methods that create an empty dataset.
     *
     * @param d The dataset description. Must have nonzero dimensions.
     * @return Reference to this RecordComponent instance.
     */
    RecordComponent& makeEmpty( Dataset d );

    /**
     * @brief Check recursively whether this RecordComponent is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool dirtyRecursive() const;

protected:
    /**
     * Make sure to parse a RecordComponent only once.
     */
    std::shared_ptr< bool > hasBeenRead = std::make_shared< bool >( false );
    /**
     * The same std::string that the parent class would pass as parameter to
     * RecordComponent::flush().
     * This is stored only upon RecordComponent::flush() if
     * AbstractIOHandler::flushLevel is set to FlushLevel::SkeletonOnly
     * (for use by the Span<T>-based overload of RecordComponent::storeChunk()).
     */
    std::shared_ptr< std::string > m_name = std::make_shared< std::string >();
}; // RecordComponent


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
        if( !isSameInteger< T >( dtype ) &&
            !isSameFloatingPoint< T >( dtype ) &&
            !isSameComplexFloatingPoint< T >( dtype ) )
            throw std::runtime_error("Type conversion during chunk loading not yet implemented");

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
inline Span< T >
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
    seriesFlush( FlushLevel::SkeletonOnly );

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
    IOHandler()->enqueue( IOTask( this, getBufferView ) );
    IOHandler()->flush();
    auto &out = *getBufferView.out;
    if( !out.taskSupportedByBackend )
    {
        auto data = std::forward< F >( createBuffer )( size );
        out.ptr = static_cast< void * >( data.get() );
        storeChunk( std::move( data ), std::move( o ), std::move( e ) );
    }
    return Span< T >{ std::move( getBufferView ), size, &writable() };
}

template< typename T >
inline Span< T >
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
} // namespace openPMD
