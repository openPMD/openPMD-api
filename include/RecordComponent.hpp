#pragma once


#include "Attributable.hpp"
#include "Dataset.hpp"
#include "Writable.hpp"


class RecordComponent : public Attributable, public Writable
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Record;
    friend class Mesh;
    friend class Iteration;

private:
    RecordComponent();

    Dataset m_dataset;
    std::queue< IOTask > m_chunks;

public:
    double unitSI() const;
    RecordComponent& setUnitSI(double);

    std::vector< double > position() const;
    RecordComponent& setPosition(std::vector< double >);

    RecordComponent& resetDataset(Dataset = Dataset());

    Datatype getDatatype();
    uint8_t getDimensionality();
    Extent getExtent();

    RecordComponent& makeConstant(/*yadda*/);

    template< typename T >
    void loadChunkInto(Offset, Extent, std::shared_ptr< T >, double targetUnitSI = 0.0);
    template< typename T >
    std::unique_ptr< T > loadChunk(Offset, Extent, double targetUnitSI = 0.0);
    template< typename T >
    void storeChunk(Offset, Extent, std::shared_ptr< T >);

    constexpr static char const * const SCALAR = "\tThisStringShouldNeverBeEnteredByHand";

private:
    void flush(std::string const&);
};  //RecordComponent

template< typename T >
inline void
RecordComponent::loadChunkInto(Offset o, Extent e, std::shared_ptr<T> data, double targetUnitSI)
{

}

template< typename T >
inline std::unique_ptr< T >
RecordComponent::loadChunk(Offset o, Extent e, double targetUnitSI)
{

}

template< typename T >
inline void
RecordComponent::storeChunk(Offset o, Extent e, std::shared_ptr<T> data)
{
    Dataset d = Dataset(data.get(), e);
    if( d.dtype != getDatatype() )
        throw std::runtime_error("Datatypes of chunk and dataset do not match.");
    uint8_t dim = getDimensionality();
    if( e.size() != dim || o.size() != dim )
        throw std::runtime_error("Dimensionality of chunk and dataset do not match.");
    Extent dse = getExtent();
    for( uint8_t i = 0; i < dim; ++i )
        if( o[i] < 0 || dse[i] < o[i] + e[i] )
            throw std::runtime_error("Chunk does not reside inside dataset.");

    Parameter< Operation::WRITE_DATASET > chunk_parameter;
    chunk_parameter.offset = o;
    chunk_parameter.extent = e;
    chunk_parameter.dtype = d.dtype;
    chunk_parameter.data = std::static_pointer_cast< void >(data);
    m_chunks.push(IOTask(this, chunk_parameter));
}

