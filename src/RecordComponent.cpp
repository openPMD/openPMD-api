#include "../include/RecordComponent.hpp"


RecordComponent::RecordComponent(bool isMeshComponent)
        : m_dataset(Dataset()),
          m_isMeshComponent{isMeshComponent}
{
    setUnitSI(1);
    if( m_isMeshComponent )
        setPosition({});
}

double
RecordComponent::unitSI() const
{
    return getAttribute("unitSI").get< double >();
}

RecordComponent&
RecordComponent::setUnitSI(double usi)
{
    setAttribute("unitSI", usi);
    return *this;
}

RecordComponent&
RecordComponent::linkData(Dataset ds, Extent e, Offset o, Direction d)
{
    m_dataset = std::move(ds);
    return *this;
}

RecordComponent&
RecordComponent::unlinkData()
{
    m_dataset.~Dataset();
    return *this;
}

std::vector< double >
RecordComponent::position() const
{
    return getAttribute("position").get< std::vector< double > >();
}

RecordComponent&
RecordComponent::setPosition(std::vector< double > pos)
{
    setAttribute("position", pos);
    return *this;
}
