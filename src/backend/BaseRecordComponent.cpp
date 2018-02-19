#include "openPMD/backend/BaseRecordComponent.hpp"


double
BaseRecordComponent::unitSI() const
{
    return getAttribute("unitSI").get< double >();
}

BaseRecordComponent&
BaseRecordComponent::resetDatatype(Datatype d)
{
    if( written )
        throw std::runtime_error("A Records Datatype can not (yet) be changed after it has been written.");

    m_dataset.dtype = d;
    return *this;
}

BaseRecordComponent::BaseRecordComponent()
        : m_dataset(Dataset(Datatype::UNDEFINED, {})),
          m_isConstant{false}
{ }

Datatype
BaseRecordComponent::getDatatype()
{
    return m_dataset.dtype;
}
