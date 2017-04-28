#include "../include/RecordComponent.hpp"


RecordComponent::RecordComponent()
        : m_dataset(Dataset())
{
    setUnitSI(1);
}

double
RecordComponent::unitSI() const
{
    return boost::get< double >(getAttribute("unitSI").getResource());
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
    return boost::get< std::vector< double > >(getAttribute("position").getResource());
}

RecordComponent&
RecordComponent::setPosition(std::vector< double > pos)
{
    setAttribute("position", pos);
    return *this;
}
