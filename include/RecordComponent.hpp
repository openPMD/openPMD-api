#pragma once


#include <type_traits>

#include "Attributable.hpp"
#include "Dataset.hpp"
#include "openPMD.hpp"



class RecordComponent : public Attributable
{
    friend class Mesh;

public:
    RecordComponent();

    double unitSI() const;
    RecordComponent& setUnitSI(double);

    void linkDataToDisk(Dataset);

    template< typename T >
    void linkDataToMemory(T* ptr, Extent ext);

    void unlinkData();

private:
    std::vector< double > position() const;
    RecordComponent& setPosition(std::vector< double >);

    Dataset m_dataset;
};

inline void
RecordComponent::linkDataToDisk(Dataset ds)
{
    m_dataset = std::move(ds);
}

/*
template< typename T >
inline T*
RecordComponent::retrieveData() const
{
    return reinterpret_cast<T*>(m_dataset.m_data);
}
 */

inline void
RecordComponent::unlinkData()
{
    m_dataset.~Dataset();
}
