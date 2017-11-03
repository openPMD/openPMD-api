#include <iostream>

#include "../include/Record.hpp"


Record::Record() = default;
Record::Record(Record const&) = default;
Record::~Record() = default;

Record&
Record::setUnitDimension(std::map< Record::UnitDimension, double > const& udim)
{
    if( !udim.empty() )
    {
        std::array< double, 7 > unitDimension = this->unitDimension();
        for( auto const& entry : udim )
            unitDimension[static_cast<uint8_t>(entry.first)] = entry.second;
        setAttribute("unitDimension", unitDimension);
        dirty = true;
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
        (*this).find(RecordComponent::SCALAR)->second.read();
    } else
    {
        clear_unchecked();
        Parameter< Operation::LIST_PATHS > plist_parameter;
        IOHandler->enqueue(IOTask(this, plist_parameter));
        IOHandler->flush();

        Parameter< Operation::OPEN_PATH > path_parameter;
        for( auto const& component : *plist_parameter.paths )
        {
            RecordComponent& rc = (*this)[component];
            path_parameter.path = component;
            IOHandler->enqueue(IOTask(&rc, path_parameter));
            IOHandler->flush();
            rc.m_isConstant = true;
            rc.read();
        }

        Parameter< Operation::LIST_DATASETS > dlist_parameter;
        IOHandler->enqueue(IOTask(this, dlist_parameter));
        IOHandler->flush();

        Parameter< Operation::OPEN_DATASET > dataset_parameter;
        for( auto const& component : *dlist_parameter.datasets )
        {
            RecordComponent& rc = (*this)[component];
            dataset_parameter.name = component;
            IOHandler->enqueue(IOTask(&rc, dataset_parameter));
            IOHandler->flush();
            rc.resetDataset(Dataset(*dataset_parameter.dtype, *dataset_parameter.extent));
            rc.read();
        }
    }

    readBase();

    readAttributes();

    /* this file need not be flushed */
    written = true;
}
