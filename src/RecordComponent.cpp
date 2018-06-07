/* Copyright 2017-2018 Fabian Koller
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Dataset.hpp"

#include <iostream>


namespace openPMD
{
RecordComponent::RecordComponent()
        : m_chunks{std::make_shared< std::queue< IOTask > >()},
          m_constantValue{std::make_shared< Attribute >(-1)}
{
    setUnitSI(1);
    resetDataset(Dataset(Datatype::CHAR, {1}));
}

RecordComponent&
RecordComponent::setUnitSI(double usi)
{
    setAttribute("unitSI", usi);
    return *this;
}

RecordComponent&
RecordComponent::resetDataset(Dataset d)
{
    if( written )
        throw std::runtime_error("A Records Dataset can not (yet) be changed after it has been written.");

    *m_dataset = d;
    dirty = true;
    return *this;
}

uint8_t
RecordComponent::getDimensionality() const
{
    return m_dataset->rank;
}

Extent
RecordComponent::getExtent() const
{
    return m_dataset->extent;
}

void
RecordComponent::flush(std::string const& name)
{
    if( IOHandler->accessType == AccessType::READ_ONLY )
    {
        while( !m_chunks->empty() )
        {
            IOHandler->enqueue(m_chunks->front());
            m_chunks->pop();
        }
    } else
    {
        if( !written )
        {
            if( *m_isConstant )
            {
                Parameter< Operation::CREATE_PATH > pCreate;
                pCreate.path = name;
                IOHandler->enqueue(IOTask(this, pCreate));
                Parameter< Operation::WRITE_ATT > aWrite;
                aWrite.name = "value";
                aWrite.dtype = m_constantValue->dtype;
                aWrite.resource = m_constantValue->getResource();
                IOHandler->enqueue(IOTask(this, aWrite));
                aWrite.name = "shape";
                Attribute a(getExtent());
                aWrite.dtype = a.dtype;
                aWrite.resource = a.getResource();
                IOHandler->enqueue(IOTask(this, aWrite));
            } else
            {
                Parameter< Operation::CREATE_DATASET > dCreate;
                dCreate.name = name;
                dCreate.extent = getExtent();
                dCreate.dtype = getDatatype();
                dCreate.chunkSize = m_dataset->chunkSize;
                dCreate.compression = m_dataset->compression;
                dCreate.transform = m_dataset->transform;
                IOHandler->enqueue(IOTask(this, dCreate));
            }
        }

        while( !m_chunks->empty() )
        {
            IOHandler->enqueue(m_chunks->front());
            m_chunks->pop();
        }

        flushAttributes();
    }
}

void
RecordComponent::read()
{
    /* allow all attributes to be set */
    written = false;

    readBase();

    /* this file need not be flushed */
    written = true;
}

void
RecordComponent::readBase()
{
    using DT = Datatype;
    Parameter< Operation::READ_ATT > aRead;

    if( *m_isConstant )
    {
        aRead.name = "value";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();

        Attribute a(*aRead.resource);
        DT dtype = *aRead.dtype;
        written = false;
        switch( dtype )
        {
            case DT::LONG_DOUBLE:
                makeConstant(a.get< long double >());
                break;
            case DT::DOUBLE:
                makeConstant(a.get< double >());
                break;
            case DT::FLOAT:
                makeConstant(a.get< float >());
                break;
            case DT::INT16:
                makeConstant(a.get< int16_t >());
                break;
            case DT::INT32:
                makeConstant(a.get< int32_t >());
                break;
            case DT::INT64:
                makeConstant(a.get< int64_t >());
                break;
            case DT::UINT16:
                makeConstant(a.get< uint16_t >());
                break;
            case DT::UINT32:
                makeConstant(a.get< uint32_t >());
                break;
            case DT::UINT64:
                makeConstant(a.get< uint64_t >());
                break;
            case DT::CHAR:
                makeConstant(a.get< char >());
                break;
            case DT::UCHAR:
                makeConstant(a.get< unsigned char >());
                break;
            case DT::BOOL:
                makeConstant(a.get< bool >());
                break;
            default:
                throw std::runtime_error("Unexpected constant datatype");
        }
        written = true;

        aRead.name = "shape";
        IOHandler->enqueue(IOTask(this, aRead));
        IOHandler->flush();
        a = Attribute(*aRead.resource);
        Extent e;
        switch( *aRead.dtype )
        {
            case DT::UINT64:
                e.push_back(a.get< uint64_t >());
                break;
            case DT::VEC_UINT64:
                for( auto const& val : a.get< std::vector< uint64_t > >() )
                    e.push_back(val);
                break;
            default:
                throw std::runtime_error("Unexpected Attribute datatype for 'shape'");
        }
        written = false;
        resetDataset(Dataset(dtype, e));
        written = true;
    }

    aRead.name = "unitSI";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    if( *aRead.dtype == DT::DOUBLE )
        setUnitSI(Attribute(*aRead.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'unitSI'");

    readAttributes();
}
} // openPMD

