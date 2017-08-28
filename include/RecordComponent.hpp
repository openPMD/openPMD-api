#pragma once


#include "Attributable.hpp"
#include "Dataset.hpp"
#include "Writable.hpp"


class RecordComponent : public Attributable
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Iteration;
    friend class ParticleSpecies;
    template< typename T_elem >
    friend class BaseRecord;
    friend class Record;
    friend class Mesh;

protected:
    RecordComponent();

    void readBase();

    Dataset m_dataset;
    std::queue< IOTask > m_chunks;
    bool m_isConstant;

public:
    double unitSI() const;
    RecordComponent& setUnitSI(double);

    RecordComponent& resetDataset(Dataset);

    Datatype getDatatype();
    uint8_t getDimensionality();
    Extent getExtent();

    template< typename T >
    RecordComponent& makeConstant(T);

    template< typename T >
    void loadChunkInto(Offset, Extent, std::shared_ptr< T >, double targetUnitSI = 0.0);
    template< typename T >
    std::unique_ptr< T, std::function< void(T*) > > loadChunk(Offset, Extent, double targetUnitSI = 0.0);
    template< typename T >
    void storeChunk(Offset, Extent, std::shared_ptr< T >);

    constexpr static char const * const SCALAR = "\tScalar";

private:
    void flush(std::string const&);
    virtual void read();
};  //RecordComponent

class MeshRecordComponent : public RecordComponent
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Mesh;

private:
    MeshRecordComponent();
    void read() override;

public:
    std::vector< float > position() const;
    MeshRecordComponent& setPosition(std::vector< float >);
};


template< typename T >
inline RecordComponent&
RecordComponent::makeConstant(T value)
{
    //TODO
    m_isConstant = true;
    return *this;
}

template< typename T >
inline void
RecordComponent::loadChunkInto(Offset o, Extent e, std::shared_ptr<T> data, double targetUnitSI)
{

}

template< typename T >
inline std::unique_ptr< T, std::function< void(T*) > >
RecordComponent::loadChunk(Offset o, Extent e, double targetUnitSI)
{
    Dataset d = Dataset(T(), {});
    if( d.dtype != getDatatype() )
        throw std::runtime_error("Type conversion not implemented yet");
    uint8_t dim = getDimensionality();
    if( e.size() != dim || o.size() != dim )
        throw std::runtime_error("Dimensionality of chunk and dataset do not match.");
    Extent dse = getExtent();
    for( uint8_t i = 0; i < dim; ++i )
        if( o[i] < 0 || dse[i] < o[i] + e[i] )
            throw std::runtime_error("Chunk does not reside inside dataset (Dimension on index " + std::to_string(i)
                                     + " - DS: " + std::to_string(dse[i])
                                     + " - Chunk: " + std::to_string(o[i] + e[i])
                                     + ")");

    Parameter< Operation::READ_DATASET > chunk_parameter;
    chunk_parameter.offset = o;
    chunk_parameter.extent = e;
    chunk_parameter.dtype = getDatatype();
    IOHandler->enqueue(IOTask(this, chunk_parameter));
    IOHandler->flush();

    T* ptr = static_cast< T* >(chunk_parameter.data.get());
    auto deleter = [](T* ptr){ delete[] ptr; };
    return std::unique_ptr< T, decltype(deleter) >(ptr, deleter);
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
            throw std::runtime_error("Chunk does not reside inside dataset (Dimension on index " + std::to_string(i)
                                     + " - DS: " + std::to_string(dse[i])
                                     + " - Chunk: " + std::to_string(o[i] + e[i])
                                     + ")");

    Parameter< Operation::WRITE_DATASET > chunk_parameter;
    chunk_parameter.offset = o;
    chunk_parameter.extent = e;
    chunk_parameter.dtype = d.dtype;
    chunk_parameter.data = std::static_pointer_cast< void >(data);
    m_chunks.push(IOTask(this, chunk_parameter));
}

