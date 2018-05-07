#include "openPMD/Record.hpp"

#include <iostream>


namespace openPMD
{
Record::Record()
{
    setTimeOffset(0.f);
}

Record::Record(Record const&) = default;
Record::~Record() = default;

Record&
Record::setUnitDimension(std::map< UnitDimension, double > const& udim)
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
            Parameter< Operation::CREATE_PATH > pCreate;
            pCreate.path = name;
            IOHandler->enqueue(IOTask(this, pCreate));
            IOHandler->flush();
            for( auto& comp : *this )
                comp.second.parent = getWritable(this);
        }
    }

    for( auto& comp : *this )
        comp.second.flush(comp.first);

    flushAttributes();
}

void
Record::read()
{
    /* allow all attributes to be set */
    written = false;

    if( m_containsScalar )
    {
        /* using operator[] will incorrectly update parent */
        this->at(RecordComponent::SCALAR).read();
    } else
    {
        clear_unchecked();
        Parameter< Operation::LIST_PATHS > pList;
        IOHandler->enqueue(IOTask(this, pList));
        IOHandler->flush();

        Parameter< Operation::OPEN_PATH > pOpen;
        for( auto const& component : *pList.paths )
        {
            RecordComponent& rc = (*this)[component];
            pOpen.path = component;
            IOHandler->enqueue(IOTask(&rc, pOpen));
            IOHandler->flush();
            rc.m_isConstant = true;
            rc.read();
        }

        Parameter< Operation::LIST_DATASETS > dList;
        IOHandler->enqueue(IOTask(this, dList));
        IOHandler->flush();

        Parameter< Operation::OPEN_DATASET > dOpen;
        for( auto const& component : *dList.datasets )
        {
            RecordComponent& rc = (*this)[component];
            dOpen.name = component;
            IOHandler->enqueue(IOTask(&rc, dOpen));
            IOHandler->flush();
            rc.written = false;
            rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            rc.written = true;
            rc.read();
        }
    }

    readBase();

    readAttributes();

    /* this file need not be flushed */
    written = true;
}
} // openPMD
