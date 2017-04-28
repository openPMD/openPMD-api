#pragma once


#include <type_traits>

#include "Attributable.hpp"
#include "Dataset.hpp"



class RecordComponent : public Attributable
{
    friend class Mesh;

public:
    RecordComponent();

    double unitSI() const;
    RecordComponent& setUnitSI(double);

    RecordComponent& linkDataToDisk(Dataset, Extent, Offset);

    RecordComponent& unlinkData();

private:
    std::vector< double > position() const;
    RecordComponent& setPosition(std::vector< double >);

    Dataset m_dataset;
};

inline RecordComponent&
RecordComponent::linkDataToDisk(Dataset ds, Extent e, Offset o)
{
    m_dataset = std::move(ds);
    return *this;
}

inline RecordComponent&
RecordComponent::unlinkData()
{
    m_dataset.~Dataset();
    return *this;
}
