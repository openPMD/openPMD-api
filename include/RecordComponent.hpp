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
    void flush();
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

}

