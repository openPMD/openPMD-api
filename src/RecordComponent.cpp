#include "../include/RecordComponent.hpp"


RecordComponent::RecordComponent(bool isMeshComponent)
        : m_dataset(Dataset()),
          m_isMeshComponent{isMeshComponent}
{
    setUnitSI(1);
    if( m_isMeshComponent )
        setPosition({});
}

double
RecordComponent::unitSI() const
{
    return getAttribute("unitSI").get< double >();
}

RecordComponent&
RecordComponent::setUnitSI(double usi)
{
    setAttribute("unitSI", usi);
    dirty = true;
    return *this;
}

RecordComponent&
RecordComponent::linkData(Extent e, Offset o, Direction d)
{
    //TODO
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
    return getAttribute("position").get< std::vector< double > >();
}

RecordComponent&
RecordComponent::setPosition(std::vector< double > pos)
{
    setAttribute("position", pos);
    dirty = true;
    return *this;
}

void
RecordComponent::flush()
{
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
}
