#include <iostream>

#include "../include/Record.hpp"


Record::Record()
  : m_containsScalar{false}
{
    setAttribute("unitDimension",
                 std::array< double, 7 >{{0., 0., 0., 0., 0., 0., 0.}});
    setTimeOffset(0);
}

Record::Record(Record const& r)
        : Container< RecordComponent >(r),
          m_containsScalar{r.m_containsScalar}
{ }

Record::~Record()
{ }

RecordComponent&
Record::operator[](std::string key)
{
    bool scalar = (key == RecordComponent::SCALAR);
    if( (scalar && size() > 0 && !m_containsScalar)
        || (m_containsScalar && !scalar) )
    {
        throw std::runtime_error("A scalar component can not be contained at "
                                 "the same time as one or more regular components.");
    }
    else
    {
        if( scalar )
            m_containsScalar = true;
        RecordComponent & ret = Container< RecordComponent >::operator[](key);
        ret.parent = this;
        return ret;
    }
}

Record::size_type
Record::erase(std::string const& key)
{
    auto res = Container< RecordComponent >::erase(key);
    if( key == RecordComponent::SCALAR || res == 0 )
        m_containsScalar = false;
    return res;
}

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
            unitDimension[static_cast<uint8_t>(entry.first)] = entry.second;
        setAttribute("unitDimension", unitDimension);
        dirty = true;
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
    dirty = true;
    return *this;
}

void
Record::flush(std::string const& name)
{
    if( !written )
    {
        if( m_containsScalar )
        {
            RecordComponent& r = at(RecordComponent::SCALAR);
            r.parent = parent;
            r.flush(name);
            abstractFilePosition = r.abstractFilePosition;
            written = true;
        } else
        {
            Parameter< Operation::CREATE_PATH > path_parameter;
            path_parameter.path = name;
            IOHandler->enqueue(IOTask(this, path_parameter));
            IOHandler->flush();
            for( auto& comp : *this )
                comp.second.parent = this;
        }
    }

    for( auto& comp : *this )
        comp.second.flush(comp.first);

    if( dirty )
    {
        Parameter< Operation::WRITE_ATT > attribute_parameter;
        for( std::string const & att_name : attributes() )
        {
            attribute_parameter.name = att_name;
            attribute_parameter.resource = getAttribute(att_name).getResource();
            attribute_parameter.dtype = getAttribute(att_name).dtype;
            IOHandler->enqueue(IOTask(this, attribute_parameter));
        }
    }

    dirty = false;
}
