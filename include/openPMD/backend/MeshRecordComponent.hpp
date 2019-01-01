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
#pragma once

#include "openPMD/RecordComponent.hpp"

#include <vector>


namespace openPMD
{
class MeshRecordComponent : public RecordComponent
{
    template<
            typename T,
            typename T_key,
            typename T_container
    >
    friend
    class Container;

    friend class Mesh;

private:
    MeshRecordComponent();
    void read() override;

public:
    template< typename T >
    std::vector< T > position() const;
    template< typename T >
    MeshRecordComponent& setPosition(std::vector< T >);

    template< typename T >
    MeshRecordComponent& makeConstant(T);
};


template< typename T >
std::vector< T >
MeshRecordComponent::position() const
{ return readVectorFloatingpoint< T >("position"); }

template< typename T >
inline MeshRecordComponent&
MeshRecordComponent::makeConstant(T value)
{
    RecordComponent::makeConstant(value);
    return *this;
}
} // openPMD
