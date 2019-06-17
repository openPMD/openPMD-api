/* Copyright 2017-2019 Fabian Koller
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
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Dataset.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <climits>


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

RecordComponent &
RecordComponent::resetDataset( Dataset d )
{
    if( written )
        throw std::runtime_error( "A record's Dataset cannot (yet) be changed "
                                  "after it has been written." );
    if( d.extent.empty() )
        throw std::runtime_error("Dataset extent must be at least 1D.");
    if( std::any_of(
            d.extent.begin(),
            d.extent.end(),
            []( Extent::value_type const & i ) { return i == 0u; } ) )
        return makeEmpty( d );

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

RecordComponent&
RecordComponent::makeEmpty( Dataset d )
{
    if( written )
        throw std::runtime_error(
            "A RecordComponent cannot (yet) be made"
            " empty after it has been written.");
    if( d.extent.size() == 0 )
        throw std::runtime_error("Dataset extent must be at least 1D.");

    *m_isEmpty = true;
    *m_dataset = d;
    dirty = true;
    static detail::DefaultValue< RecordComponent > dv;
    switchType(
        d.dtype,
        dv,
        *this );
    return *this;
}

void
RecordComponent::flush(std::string const& name)
{
    if( IOHandler->accessTypeFrontend == AccessType::READ_ONLY )
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
    readBase();
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
            case DT::SHORT:
                makeConstant(a.get< short >());
                break;
            case DT::INT:
                makeConstant(a.get< int >());
                break;
            case DT::LONG:
                makeConstant(a.get< long >());
                break;
            case DT::LONGLONG:
                makeConstant(a.get< long long >());
                break;
            case DT::USHORT:
                makeConstant(a.get< unsigned short >());
                break;
            case DT::UINT:
                makeConstant(a.get< unsigned int >());
                break;
            case DT::ULONG:
                makeConstant(a.get< unsigned long >());
                break;
            case DT::ULONGLONG:
                makeConstant(a.get< unsigned long long >());
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

        // uint64_t check
        Datatype const attrDtype = *aRead.dtype;
        if( isSame( attrDtype, determineDatatype< uint64_t >() ) )
            e.push_back( a.get< uint64_t >() );
        else if( isSame( attrDtype, determineDatatype< std::vector< uint64_t > >() ) )
            for( auto const& val : a.get< std::vector< uint64_t > >() )
                e.push_back( val );
        else
        {
            std::ostringstream oss;
            oss << "Unexpected datatype ("
                << *aRead.dtype
                << ") for attribute 'shape' ("
                << determineDatatype< uint64_t >()
                << " aka uint64_t)";
            throw std::runtime_error(oss.str());
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

