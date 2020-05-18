/* Copyright 2017-2020 Fabian Koller
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
#include "openPMD/backend/MeshGeometry.hpp"
#include "openPMD/backend/MeshDataOrder.hpp"

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
    ~MeshRecordComponent() override = default;

    /** Position on an element
     *
     * Relative on an element (node/cell/voxel) of the mesh
     *
     * @return relative position within range of [0.0:1.0)
     */
    template< typename T >
    std::vector< T > position() const;

    /** Position on an element
     *
     * Relative on an element (node/cell/voxel) of the mesh
     *
     * @param[in] pos relative position in range [0.0:1.0)
     */
    template< typename T >
    MeshRecordComponent& setPosition(std::vector< T > pos);

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
    template< typename T >
    MeshRecordComponent& makeConstant(T);

    // below are read-only attributes from Mesh, duplicated for user read-convenience

    /** @brief Enumerated datatype for the geometry of the mesh.
     *
     * @note If the default values do not suit your application, you can set arbitrary
     *       Geometry with Mesh::setAttribute("geometry", VALUE).
     *       Note that this might break openPMD compliance and tool support.
     */
    using Geometry = MeshGeometry;

    /** @brief Enumerated datatype for the memory layout of N-dimensional data.
     */
    using DataOrder = MeshDataOrder;

    /**
     * @return String representing the geometry of the mesh of the mesh record.
     */
    Geometry geometry() const;

    /**
     * @throw   no_such_attribute_error If Mesh::geometry is not Mesh::Geometry::thetaMode.
     * @return  String representing additional parameters for the geometry, separated by a @code ; @endcode.
     */
    std::string geometryParameters() const;

    /**
     * @return  Memory layout of N-dimensional data.
     */
    DataOrder dataOrder() const;

    /**
     * @return  Ordering of the labels for the Mesh::geometry of the mesh.
     */
    std::vector< std::string > axisLabels() const;

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float, double).
     * @return  vector of T representing the spacing of the grid points along each dimension (in the units of the simulation).
     */
    template< typename T >
    std::vector< T > gridSpacing() const;

    /**
     * @return  Vector of (double) representing the start of the current domain of the simulation (position of the beginning of the first cell) in simulation units.
     */
    std::vector< double > gridGlobalOffset() const;

    /**
     * @return  Unit-conversion factor to multiply each value in Mesh::gridSpacing and Mesh::gridGlobalOffset, in order to convert from simulation units to SI units.
     */
    double gridUnitSI() const;

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float, double).
     * @return  Offset between the time at which this record is defined and the Iteration::time attribute of the Series::basePath level.
     */
    template< typename T >
    T timeOffset() const;
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

template< typename T >
inline std::vector< T >
MeshRecordComponent::gridSpacing() const
{ return m_writable->parent->attributable->readVectorFloatingpoint< T >("gridSpacing"); }

template< typename T >
inline T
MeshRecordComponent::timeOffset() const
{ return m_writable->parent->attributable->readFloatingpoint< T >("timeOffset"); }

} // namespace openPMD
