#include "backend/PatchRecordComponent.hpp"

GenericPatchData&
PatchRecordComponent::operator[](PatchPosition const& pos)
{
    return m_data[pos];
}

PatchRecordComponent&
PatchRecordComponent::setUnitSI(double usi)
{
    setAttribute("unitSI", usi);
    dirty = true;
    return *this;
}

PatchRecordComponent::PatchRecordComponent()
{
    setUnitSI(1);
}

void
PatchRecordComponent::flush(std::string const& name)
{
    if( !written )
    {
        Parameter< Operation::CREATE_DATASET > dCreate;
        dCreate.name = name;
        dCreate.dtype = getDatatype();
        dCreate.extent = {1};
        IOHandler->enqueue(IOTask(this, dCreate));
    }
    IOHandler->flush();
}
