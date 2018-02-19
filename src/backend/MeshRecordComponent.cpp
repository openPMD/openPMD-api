#include "openPMD/backend/MeshRecordComponent.hpp"


MeshRecordComponent::MeshRecordComponent()
        : RecordComponent()
{
    setPosition(std::vector< double >{0});
}

void
MeshRecordComponent::read()
{
    /* allow all attributes to be set */
    written = false;

    using DT = Datatype;
    Parameter< Operation::READ_ATT > aRead;

    aRead.name = "position";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    Attribute a = Attribute(*aRead.resource);
    if( *aRead.dtype == DT::VEC_FLOAT )
        setPosition(a.get< std::vector< float > >());
    else if( *aRead.dtype == DT::VEC_DOUBLE )
        setPosition(a.get< std::vector< double > >());
    else if( *aRead.dtype == DT::VEC_LONG_DOUBLE )
        setPosition(a.get< std::vector< long double > >());
    else
        throw std::runtime_error( "Unexpected Attribute datatype for 'position'");

    readBase();

    /* this file need not be flushed */
    written = true;
}

template< typename T >
MeshRecordComponent&
MeshRecordComponent::setPosition(std::vector< T > pos)
{
    static_assert(std::is_floating_point< T >::value,
                  "Type of attribute must be floating point");

    setAttribute("position", pos);
    return *this;
}

template
MeshRecordComponent&
MeshRecordComponent::setPosition(std::vector< float > pos);
template
MeshRecordComponent&
MeshRecordComponent::setPosition(std::vector< double > pos);
template
MeshRecordComponent&
MeshRecordComponent::setPosition(std::vector< long double > pos);
