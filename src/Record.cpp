#include <iostream>

#include "../include/Record.hpp"


Record::Record(Dimension dim,
               std::initializer_list< std::string > comps,
               bool isRecordComponent)
        : m_components{std::map< std::string, RecordComponent >()},
          m_isComponent{isRecordComponent},
          m_dimension{dim}
{
    for( std::string const& component : comps )
    {
        using it_t = std::map< std::string, RecordComponent >::iterator;
        std::pair< it_t, bool > res =
                m_components.emplace(component,
                                     RecordComponent());
        res.first->second.setAttribute("unitSI", static_cast<double>(1));
    }
    setAttribute("unitDimension",
                 std::array< double, 7 >{{0., 0., 0., 0., 0., 0., 0.}});
    setTimeOffset(0);
    if( m_isComponent )
    {
        setUnitSI(1);
    }
}

Record::Record(Record const& r)
        : Attributable(r),
          m_components{r.m_components},
          m_isComponent{r.m_isComponent},
          m_dimension{r.m_dimension}
{ }

std::array< double, 7 >
Record::unitDimension() const
{
    return boost::get< std::array< double, 7 > >(getAttribute("unitDimension").getResource());
}

Record&
Record::setUnitDimension(std::map< Record::UnitDimension, double > const& udim)
{
    if( udim.size() != 0 )
    {
        std::array< double, 7 > unitDimension = this->unitDimension();
        for( auto const& entry : udim )
        {
            unitDimension[static_cast<size_t>(entry.first)] = entry.second;
        }
        setAttribute("unitDimension", unitDimension);
    }
    return *this;
}

float
Record::timeOffset() const
{
    return boost::get< float >(getAttribute("timeOffset").getResource());
}

Record&
Record::setTimeOffset(float timeOffset)
{
    setAttribute("timeOffset", timeOffset);
    return *this;
}

double
Record::unitSI() const
{
    if( m_isComponent )
    {
        return boost::get< double >(getAttribute("unitSI").getResource());
    } else
    {
        std::cerr<<"You have to use "
                 <<"\'record[\"component\"].unitSI()\'"
                 <<" if you want to get unitSI on skalar records!\n";
        return 1;
    }
}

Record&
Record::setUnitSI(double usi)
{
    if( m_isComponent )
    {
        setAttribute("unitSI", usi);
    } else
    {
        std::cerr<<"You have to use "
                 <<"\'record[\"component\"].setUnitSI("<<usi<<")\'"
                 <<" if you want to set unitSI on skalar records!\n";
    }
    return *this;
}

RecordComponent&
Record::operator[](std::string const& component)
{
    assert(!m_isComponent);
    auto it = m_components.find(component);
    if( it != m_components.end() )
    {
        return it->second;
    } else
    {
        std::cerr<<"Unknown record component."<<std::endl;
    }
}
