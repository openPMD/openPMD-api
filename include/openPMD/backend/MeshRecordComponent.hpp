/* Copyright 2017-2021 Fabian Koller
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
    template <typename T, typename T_key, typename T_container>
    friend class Container;

    friend class Mesh;

private:
    MeshRecordComponent();
    void read() override;

public:
    ~MeshRecordComponent() override = default;

    /** Position on an element
     *
     * Relative on an element (node/cell/voxel) of the mesh
     *
     * @return relative position within range of [0.0:1.0)
     */
    template <typename T>
    std::vector<T> position() const;

    /** Position on an element
     *
     * Relative on an element (node/cell/voxel) of the mesh
     *
     * @param[in] pos relative position in range [0.0:1.0)
     */
    template <typename T>
    MeshRecordComponent &setPosition(std::vector<T> pos);

    /** Create a dataset with regular extent and constant value
     *
     * In a constant record component, the value for each date in its extent is
     * the same. Implemented by storing only a constant value as meta-data.
     *
     * @see RecordComponent::makeConstant
     *
     * @tparam T type of the stored value
     * @return A reference to this RecordComponent.
     */
    template <typename T>
    MeshRecordComponent &makeConstant(T);
};

template <typename T>
std::vector<T> MeshRecordComponent::position() const
{
    return readVectorFloatingpoint<T>("position");
}

template <typename T>
inline MeshRecordComponent &MeshRecordComponent::makeConstant(T value)
{
    RecordComponent::makeConstant(value);
    return *this;
}
} // namespace openPMD
