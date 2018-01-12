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
        dCreate.extent = {1};
        dCreate.dtype = getDatatype();
        dCreate.chunkSize = {1};
        dCreate.compression = m_dataset.compression;
        dCreate.transform = m_dataset.transform;
        IOHandler->enqueue(IOTask(this, dCreate));
    }
    IOHandler->flush();
}
