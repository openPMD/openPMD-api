#include <iostream>

#include "../include/Record.hpp"


Record
Record::makeScalarRecord(Extent ext)
{
    return Record(ext, {}, true);
}

Record
Record::makeTensorRecord(Extent ext,
                         std::initializer_list< std::string > il)
{
    return Record(ext, il, il.size() == 0);
}

Record::Record(Extent ext,
               std::initializer_list< std::string > comps,
               bool isRecordComponent)
        : m_isComponent{isRecordComponent},
          m_extent{ext}
{
    for( std::string const& component : comps )
    {
        using it_t = Container::iterator;
        std::pair< it_t, bool > res = emplace(component, RecordComponent());
        res.first->second.setAttribute("unitSI", static_cast<double>(1));
    }
    setAttribute("unitDimension",
                 std::array< double, 7 >{{0., 0., 0., 0., 0., 0., 0.}});
    setTimeOffset(0);
}

Record::Record(Record const& r)
        : Container< RecordComponent >(r),
          scalar{r.scalar},
          m_isComponent{r.m_isComponent},
          m_extent{r.m_extent}
{ }

Record::Record()
        : Record(Extent{1, 1, 1}, {}, true)
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
