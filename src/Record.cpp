#include "../include/Record.hpp"


Record::Record(Dimension dim, std::initializer_list< std::string > comps)
        : m_components{std::map< Record::Component, Record >()}
{
    for( std::string const& component : comps )
    {
        std::pair< iterator, bool > result = m_components.emplace(component,
                                                                  Record(dim,
                                                                         {}));
    }

}

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
    using array_t = std::array< double, 7 >;
    return boost::get< array_t >(getAttribute("unitDimension"));
}

Record
Record::setUnitDimension(std::map< Record::UnitDimension, double > udim)
{
    std::array< double, 7 > unitDimension;
    for( auto const& entry : udim )
    {
        unitDimension[static_cast<size_t>(entry.first)] = entry.second;
    }
    setAttribute("unitDimension", unitDimension);
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

std::map< Record::Component, double >
Record::unitSI() const
{
    std::map< Record::Component, double > ret;
    for( auto const& entry : m_components )
    {
        ret.emplace({entry.first,
                     boost::get< double >(getAttribute("unitSI"))});
    }
    return ret;
}

Record
Record::setUnitSI(std::map< Component, double > usi)
{
    for( auto const& entry : usi )
    {
        m_components[entry.first].setAttribute("unitSI", entry.second);
    }
    return *this;
}
