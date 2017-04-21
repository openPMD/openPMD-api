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

    RecordComponent& linkDataToDisk(Dataset);

    template< typename T >
    void linkDataToMemory(T* ptr, Extent ext);

    RecordComponent& unlinkData();

private:
    std::vector< double > position() const;
    RecordComponent& setPosition(std::vector< double >);

    Dataset m_dataset;
};

inline RecordComponent&
RecordComponent::linkDataToDisk(Dataset ds)
{
    m_dataset = std::move(ds);
    return *this;
}

/*
template< typename T >
inline T*
RecordComponent::retrieveData() const
{
    return reinterpret_cast<T*>(m_dataset.m_data);
}
 */

inline RecordComponent&
RecordComponent::unlinkData()
{
    m_dataset.~Dataset();
    return *this;
}
