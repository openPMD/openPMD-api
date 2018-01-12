#include "auxiliary/Memory.hpp"
#include "backend/PatchRecord.hpp"


GenericPatchData::GenericPatchData()
        : m_data(0)
{
    m_data.dtype = Dtype::UNDEFINED;
}

PatchRecord&
PatchRecord::setUnitDimension(std::map< UnitDimension, double > const& udim)
{
    if( !udim.empty() )
    {
        std::array< double, 7 > unitDimension = this->unitDimension();
        for( auto const& entry : udim )
            unitDimension[static_cast<uint8_t>(entry.first)] = entry.second;
        setAttribute("unitDimension", unitDimension);
    }
    return *this;
}

PatchRecord::PatchRecord()
{ }

void
PatchRecord::flush(std::string const& path)
{
    Container< PatchRecordComponent >::flush(path);

    for( auto& comp : *this )
        comp.second.flush(comp.first);
}

void
PatchRecord::read()
{
    Parameter< Operation::LIST_DATASETS > dList;
    IOHandler->enqueue(IOTask(this, dList));
    IOHandler->flush();

    Parameter< Operation::OPEN_DATASET > dOpen;
    Parameter< Operation::READ_DATASET > dRead;
    for( auto const& component_name : *dList.datasets )
    {
        PatchRecordComponent& prc = (*this)[component_name];
        dOpen.name = component_name;
        IOHandler->enqueue(IOTask(&prc, dOpen));
        IOHandler->flush();

        dRead.dtype = *dOpen.dtype;
        dRead.extent = *dOpen.extent;
        dRead.offset = {0};

        size_t numPoints = dRead.extent[0];
        auto data = allocatePtr(dRead.dtype, numPoints);
        dRead.data = data.get();
        IOHandler->enqueue(IOTask(&prc, dRead));
        IOHandler->flush();
    }
}
