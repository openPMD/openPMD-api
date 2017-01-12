#include <iostream>

#include "../include/Record.hpp"


Record::Record(std::string name,
               Dimension dim,
               std::initializer_list< std::string > comps,
               bool isRecordComponent)
        : m_components{std::map< std::string, Record >()},
          m_isComponent{isRecordComponent},
          m_dimension{dim}
{
    for( std::string const& component : comps )
    {
        using it_t = std::map< std::string, Record >::iterator;
        std::pair< it_t, bool > res = m_components.emplace(component,
                                                           Record(component,
                                                                  dim,
                                                                  {},
                                                                  true));
        res.first->second.setAttribute("unitSI", static_cast<double>(1));
    }
    setName(name);
    if( !m_isComponent )
    {
        setAttribute("unitDimension",
                     std::array< double, 7 >{{0., 0., 0., 0., 0., 0., 0.}});
        setTimeOffset(0);
    }
}

Record::Record(Record const& r)
        : Attributable(r),
          m_components{r.m_components},
          m_isComponent{r.m_isComponent},
          m_dimension{r.m_dimension}
{ }

std::string const
Record::name() const
{
    return boost::get< std::string >(getAttribute("name"));
}

Record
Record::setName(std::string const& n)
{
    setAttribute("name", n);
    return *this;
}

std::array< double, 7 >
Record::unitDimension() const
{
    return boost::get< std::array< double, 7 > >(getAttribute("unitDimension"));
}

Record
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
    return boost::get< float >(getAttribute("timeOffset"));
}

Record
Record::setTimeOffset(float timeOffset)
{
    setAttribute("timeOffset", timeOffset);
    return *this;
}

double
Record::unitSI() const
{
    return boost::get< double >(getAttribute("unitSI"));
}

Record
Record::setUnitSI(double usi)
{
    assert(m_isComponent);
    setAttribute("unitSI", usi);
    return *this;
}

Record
Record::setUnitSI(std::map< std::string, double > usi)
{
    assert(!m_isComponent);
    for( auto const& entry : usi )
    {
        auto it = m_components.find(entry.first);
        if( it != m_components.end() )
        {
            it->second.setAttribute("unitSI", entry.second);
        } else
        {
            std::cerr<<"Unknown record component."<<std::endl;
        }
    }
    return *this;
}

Record&
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

void
Record::unlinkData()
{ }
