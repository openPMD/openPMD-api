#include "../include/RecordComponent.hpp"


RecordComponent::RecordComponent()
        : rank{0},
          extents{},
          dtype{Dtype::UNDEFINED},
          m_data{nullptr}
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
