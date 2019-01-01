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
#include "openPMD/backend/MeshRecordComponent.hpp"


namespace openPMD
{
MeshRecordComponent::MeshRecordComponent()
        : RecordComponent()
{
    setPosition(std::vector< double >{0});
}

void
MeshRecordComponent::read()
{
    using DT = Datatype;
    Parameter< Operation::READ_ATT > aRead;

    aRead.name = "position";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    Attribute a = Attribute(*aRead.resource);
    if( *aRead.dtype == DT::VEC_FLOAT )
        setPosition(a.get< std::vector< float > >());
    else if( *aRead.dtype == DT::FLOAT )
        setPosition(std::vector< float >({a.get< float >()}));
    else if( *aRead.dtype == DT::VEC_DOUBLE )
        setPosition(a.get< std::vector< double > >());
    else if( *aRead.dtype == DT::DOUBLE )
        setPosition(std::vector< double >({a.get< double >()}));
    else if( *aRead.dtype == DT::VEC_LONG_DOUBLE )
        setPosition(a.get< std::vector< long double > >());
    else if( *aRead.dtype == DT::LONG_DOUBLE )
        setPosition(std::vector< long double >({a.get< long double >()}));
    else
        throw std::runtime_error( "Unexpected Attribute datatype for 'position'");

    readBase();
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
} // openPMD
