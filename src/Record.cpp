#include <iostream>

#include "../include/Record.hpp"


Record::Record() = default;
Record::Record(Record const& r) = default;
Record::~Record() = default;

RecordComponent&
Record::operator[](std::string key)
{
    bool scalar = (key == RecordComponent::SCALAR);
    if( (scalar && !empty() && !m_containsScalar)
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

float
Record::timeOffset() const
{
    return getAttribute("timeOffset").get< float >();
}

Record&
Record::setTimeOffset(float to)
{
    setAttribute("timeOffset", to);
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

    flushAttributes();
}

void
Record::read()
{
    if( m_containsScalar )
    {
        /* using operator[] will falsely update parent */
        (*this).find(RecordComponent::SCALAR)->second.read();
    } else
    {
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
}
