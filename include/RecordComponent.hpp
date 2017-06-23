#pragma once


#include "Attributable.hpp"
#include "Dataset.hpp"
#include "Writable.hpp"


enum class Direction
{
    MemoryToDisk,
    DiskToMemory
};  //Direction


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
    RecordComponent(bool isMeshComponent = false);

    Dataset m_dataset;
    bool m_isMeshComponent;

public:
    double unitSI() const;
    RecordComponent& setUnitSI(double);

    std::vector< double > position() const;
    RecordComponent& setPosition(std::vector< double >);

    RecordComponent& makeConstant(/*yadda*/);

    RecordComponent& resetDataset(Dataset);
    RecordComponent& linkData(Extent, Offset, Direction);

    RecordComponent& unlinkData();

    constexpr static char const * const SCALAR = "\tThisStringShouldNeverBeEnteredByHand";

private:
    void flush();
};  //RecordComponent
