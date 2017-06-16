#include <iostream>

#include "../include/Record.hpp"


Record::Record()
{
    setAttribute("unitDimension",
                 std::array< double, 7 >{{0., 0., 0., 0., 0., 0., 0.}});
    setTimeOffset(0);
}

Record::Record(Record const& r)
        : Container< RecordComponent >(r)
{ }

Record::~Record()
{ }

std::array< double, 7 >
Record::unitDimension() const
{
    return getAttribute("unitDimension").get< std::array< double, 7 > >();
}

Record&
Record::setUnitDimension(std::map< Record::UnitDimension, double > const& udim)
{
    if( udim.size() != 0 )
    {
        std::array< double, 7 > unitDimension = this->unitDimension();
        for( auto const& entry : udim )
        {
            unitDimension[static_cast<uint8_t>(entry.first)] = entry.second;
        }
        setAttribute("unitDimension", unitDimension);
    }
    return *this;
}

float
Record::timeOffset() const
{
    return getAttribute("timeOffset").get< float >();
}

Record&
Record::setTimeOffset(float timeOffset)
{
    setAttribute("timeOffset", timeOffset);
    return *this;
}
