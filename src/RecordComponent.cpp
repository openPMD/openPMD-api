#include "../include/RecordComponent.hpp"


RecordComponent::RecordComponent()
        : extents{{1, 1, 1}},
          m_data{nullptr}
{
    setUnitSI(1);
}

double
RecordComponent::unitSI() const
{
    return boost::get< double >(getAttribute("unitSI"));
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
    return boost::get< std::vector< double > >(getAttribute("position"));
}

RecordComponent&
RecordComponent::setPosition(std::vector< double > pos)
{
    setAttribute("position", pos);
    return *this;
}

void
RecordComponent::unlinkData()
{
    m_data = nullptr;
}
