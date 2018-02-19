#include "openPMD/RecordComponent.hpp"

#include <iostream>


RecordComponent::RecordComponent()
        : m_constantValue{-1}
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

    m_dataset = d;
    dirty = true;
    return *this;
}

uint8_t
RecordComponent::getDimensionality()
{
    return m_dataset.rank;
}

Extent
RecordComponent::getExtent()
{
    return m_dataset.extent;
}

void
RecordComponent::flush(std::string const& name)
{
    if( !written )
    {
        if( m_isConstant )
        {
            Parameter< Operation::CREATE_PATH > pCreate;
            pCreate.path = name;
            IOHandler->enqueue(IOTask(this, pCreate));
            Parameter< Operation::WRITE_ATT > aWrite;
            aWrite.name = "value";
            aWrite.dtype = m_constantValue.dtype;
            aWrite.resource = m_constantValue.getResource();
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
            dCreate.chunkSize = m_dataset.chunkSize;
            dCreate.compression = m_dataset.compression;
            dCreate.transform = m_dataset.transform;
            IOHandler->enqueue(IOTask(this, dCreate));
        }
        IOHandler->flush();
    }

    while( !m_chunks.empty() )
    {
        IOHandler->enqueue(m_chunks.front());
        m_chunks.pop();
        IOHandler->flush();
    }

    flushAttributes();
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

    if( m_isConstant )
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

