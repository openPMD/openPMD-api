#include "../include/RecordComponent.hpp"


RecordComponent::RecordComponent()
        : m_dataset(Dataset())
{
    setUnitSI(1);
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

RecordComponent&
RecordComponent::resetDataset(Dataset d)
{
    m_dataset = d;
    dirty = true;
    return *this;
}

Datatype
RecordComponent::getDatatype()
{
    return m_dataset.dtype;
}

uint8_t
RecordComponent::getDimensionality()
{
    return m_dataset.rank;
}

Extent
RecordComponent::getExtent()
{
    return m_dataset.extents;
}

RecordComponent&
RecordComponent::makeConstant()
{
    //TODO
    return *this;
}

void
RecordComponent::flush(std::string const& name)
{
    if( !written )
    {
        Parameter< Operation::CREATE_DATASET > ds_parameter;
        ds_parameter.name = name;
        ds_parameter.dtype = getDatatype();
        ds_parameter.extent = getExtent();
        IOHandler->enqueue(IOTask(this, ds_parameter));
        IOHandler->flush();
    }

    while( !m_chunks.empty() )
    {
        IOHandler->enqueue(m_chunks.front());
        m_chunks.pop();
        IOHandler->flush();
    }

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
