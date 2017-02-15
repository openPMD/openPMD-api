#include "../include/RecordComponent.hpp"


RecordComponent::RecordComponent()
        : extents{{1, 1, 1}}, m_data{nullptr}
{
    setUnitSI(1);
}

double
RecordComponent::unitSI() const
{
    return boost::get< double >(getAttribute("unitSI"));
}

RecordComponent
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

RecordComponent
RecordComponent::setPosition(std::vector< double > pos)
{
    setAttribute("position", pos);
    return *this;
}

template< typename T >
void
RecordComponent::linkData(T* ptr, std::vector< std::size_t > ext)
{
    assert(0 < ext.size() && ext.size() < 4);
    m_data = reinterpret_cast<unsigned char*>(ptr);
    for( int i = 0; i < ext.size(); ++i )
    {
        extents[2 - i] = ext[ext.size() - 1 - i];
    }
}

template< typename T >
T*
RecordComponent::retrieveData()
{
    return reinterpret_cast<T*>(m_data);
}

void
RecordComponent::unlinkData()
{
    m_data = nullptr;
}
