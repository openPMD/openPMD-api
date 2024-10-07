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

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"

#include <array>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

namespace openPMD
{
/** @brief Container for N-dimensional, homogeneous Records.
 *
 * @see
 * https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#mesh-based-records
 */
class Mesh : public BaseRecord<MeshRecordComponent>
{
    friend class Container<Mesh>;
    friend class Iteration;

public:
    Mesh(Mesh const &) = default;
    Mesh &operator=(Mesh const &) = default;
    ~Mesh() override = default;

    /** @brief Enumerated datatype for the geometry of the mesh.
     *
     * @note If the default values do not suit your application, you can set
     * arbitrary Geometry with MeshRecordComponent::setAttribute("geometry",
     * VALUE). Note that this might break openPMD compliance and tool support.
     */
    enum class Geometry
    {
        cartesian,
        thetaMode,
        cylindrical,
        spherical,
        other
    }; // Geometry

    /** @brief Enumerated datatype for the memory layout of N-dimensional data.
     */
    enum class DataOrder : char
    {
        C = 'C',
        F = 'F'
    }; // DataOrder

    /**
     * @return Enum representing the geometry of the mesh of the mesh record.
     */
    Geometry geometry() const;
    /**
     * @return String representing the geometry of the mesh of the mesh record.
     */
    std::string geometryString() const;
    /** Set the geometry of the mesh of the mesh record.
     *
     * @param   g    geometry of the mesh of the mesh record.
     * @return  Reference to modified mesh.
     */
    Mesh &setGeometry(Geometry g);
    /** Set the geometry of the mesh of the mesh record.
     *
     * If the geometry is unknown to the openPMD-api, the string is prefixed
     * with "other:" automatically unless the prefix is already present.
     *
     * @param   geometry    geometry of the mesh of the mesh record, as string
     * @return  Reference to modified mesh.
     */
    Mesh &setGeometry(std::string geometry);

    /**
     * @throw   no_such_attribute_error If Mesh::geometry is not
     * Mesh::Geometry::thetaMode.
     * @return  String representing additional parameters for the geometry,
     * separated by a @code ; @endcode.
     */
    std::string geometryParameters() const;
    /** Set additional parameters for the geometry, separated by a @code ;
     * @endcode.
     *
     * @note    Separation constraint is not verified by API.
     * @param   geometryParameters  additional parameters for the geometry,
     * separated by a @code ; @endcode.
     * @return  Reference to modified mesh.
     */
    Mesh &setGeometryParameters(std::string const &geometryParameters);

    /**
     * @return  Memory layout of N-dimensional data.
     */
    DataOrder dataOrder() const;
    /** Set the memory layout of N-dimensional data.
     *
     * @param   dor   memory layout of N-dimensional data.
     * @return  Reference to modified mesh.
     */
    Mesh &setDataOrder(DataOrder dor);

    /**
     * @return  Ordering of the labels for the Mesh::geometry of the mesh.
     */
    std::vector<std::string> axisLabels() const;
    /** Set the ordering of the labels for the Mesh::geometry of the mesh.
     *
     * @note    Dimensionality constraint is not verified by API.
     * @param   axisLabels  vector containing N (string) elements, where N is
     * the number of dimensions in the simulation.
     * @return  Reference to modified mesh.
     */
    Mesh &setAxisLabels(std::vector<std::string> const &axisLabels);

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float,
     * double).
     * @return  vector of T representing the spacing of the grid points along
     * each dimension (in the units of the simulation).
     */
    template <typename T>
    std::vector<T> gridSpacing() const;
    /** Set the spacing of the grid points along each dimension (in the units of
     * the simulation).
     *
     * @note    Dimensionality constraint is not verified by API.
     * @tparam  T   Floating point type of user-selected precision (e.g. float,
     * double).
     * @param   gridSpacing     vector containing N (T) elements, where N is the
     * number of dimensions in the simulation.
     * @return  Reference to modified mesh.
     */
    template <
        typename T,
        typename = std::enable_if_t<std::is_floating_point<T>::value>>
    Mesh &setGridSpacing(std::vector<T> const &gridSpacing);

    /**
     * @return  Vector of (double) representing the start of the current domain
     * of the simulation (position of the beginning of the first cell) in
     * simulation units.
     */
    std::vector<double> gridGlobalOffset() const;
    /** Set the start of the current domain of the simulation (position of the
     * beginning of the first cell) in simulation units.
     *
     * @note    Dimensionality constraint is not verified by API.
     * @param   gridGlobalOffset    vector containing N (double) elements, where
     * N is the number of dimensions in the simulation.
     * @return  Reference to modified mesh.
     */
    Mesh &setGridGlobalOffset(std::vector<double> const &gridGlobalOffset);

    /**
     * @return  Unit-conversion factor to multiply each value in
     * Mesh::gridSpacing and Mesh::gridGlobalOffset, in order to convert from
     * simulation units to SI units.
     */
    double gridUnitSI() const;
    /** Set the unit-conversion factor to multiply each value in
     * Mesh::gridSpacing and Mesh::gridGlobalOffset, in order to convert from
     * simulation units to SI units.
     *
     * Valid for openPMD version 1.*.
     * In order to specify the gridUnitSI per dimension (openPMD 2.*),
     * use the vector overload or `setGridUnitSIPerDimension()`.
     *
     * @param   gridUnitSI  unit-conversion factor to multiply each value in
     * Mesh::gridSpacing and Mesh::gridGlobalOffset, in order to convert from
     * simulation units to SI units.
     * @return  Reference to modified mesh.
     */
    Mesh &setGridUnitSI(double gridUnitSI);

    /** Alias for `setGridUnitSI(std::vector<double>)`.
     *
     * Set the unit-conversion factor per dimension to multiply each value in
     * Mesh::gridSpacing and Mesh::gridGlobalOffset, in order to convert from
     * simulation units to SI units.
     *
     * Valid for openPMD 2.*.
     * The legacy behavior (openPMD 1.*, a scalar gridUnitSI) is implemented
     * by `setGridUnitSI(double)`.
     *
     * @param   gridUnitSI  unit-conversion factor to multiply each value in
     * Mesh::gridSpacing and Mesh::gridGlobalOffset, in order to convert from
     * simulation units to SI units.
     *
     * @return  Reference to modified mesh.
     */
    Mesh &setGridUnitSI(std::vector<double> gridUnitSI);

    /**
     * @return  A vector of the gridUnitSI per grid dimension as defined
     * by the axisLabels. If the gridUnitSI is defined as a scalar
     * (legacy openPMD), the dimensionality is determined and a vector of
     * `dimensionality` times the scalar vector is returned.
     */
    std::vector<double> gridUnitSIPerDimension() const;

    /* Set the unit-conversion factor per dimension to multiply each value in
     * Mesh::gridSpacing and Mesh::gridGlobalOffset, in order to convert from
     * simulation units to SI units.
     *
     * Valid for openPMD 2.*.
     * The legacy behavior (openPMD 1.*, a scalar gridUnitSI) is implemented
     * by `setGridUnitSI(double)`.
     *
     * @param   gridUnitSI  unit-conversion factor to multiply each value in
     * Mesh::gridSpacing and Mesh::gridGlobalOffset, in order to convert from
     * simulation units to SI units.
     *
     * @return  Reference to modified mesh.
     */
    Mesh &setGridUnitSIPerDimension(std::vector<double> gridUnitSI);

    /** Set the powers of the 7 base measures characterizing the record's unit
     * in SI.
     *
     * @param   unitDimension   map containing pairs of (UnitDimension, double)
     * that represent the power of the particular base.
     * @return  Reference to modified mesh.
     */
    Mesh &
    setUnitDimension(std::map<UnitDimension, double> const &unitDimension);

    /**
     * @brief Set the unitDimension for each axis of the current grid.
     *
     * @param gridUnitDimension A vector of the unitDimensions for each
     * axis of the grid in the order of the axisLabels.

     * Behavior note: This is an updating method, meaning that an SI unit that
     * has been defined before and is in the next call not explicitly set
     * in the `std::map<UnitDimension, double>` will keep its previous value.
     *
     * @return Reference to modified mesh.
     */
    Mesh &setGridUnitDimension(
        std::vector<std::map<UnitDimension, double>> const &gridUnitDimension);

    /**
     * @brief Return the physical dimensions of the mesh axes.

     * If the attribute is not defined, the axes are assumed to be spatial
     * and the return value will be according to this assumption.
     * If the attribute is defined, the dimensionality of the return value is
     * not checked against the dimensionality of the mesh.
     *
     * @return A vector of arrays, each array representing the SI unit of one
     * mesh axis.
     */
    std::vector<std::array<double, 7>> gridUnitDimension() const;

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float,
     * double).
     * @return  Offset between the time at which this record is defined and the
     * Iteration::time attribute of the Series::basePath level.
     */
    template <typename T>
    T timeOffset() const;
    /** Set the offset between the time at which this record is defined and the
     * Iteration::time attribute of the Series::basePath level.
     *
     * @note    This should be written in the same unit system as
     * Iteration::time.
     * @tparam  T   Floating point type of user-selected precision (e.g. float,
     * double).
     * @param   timeOffset  Offset between the time at which this record is
     * defined and the Iteration::time attribute of the Series::basePath level.
     * @return  Reference to modified mesh.
     */
    template <
        typename T,
        typename = std::enable_if_t<std::is_floating_point<T>::value>>
    Mesh &setTimeOffset(T timeOffset);

private:
    Mesh();

    void
    flush_impl(std::string const &, internal::FlushParams const &) override;
    void read();
}; // Mesh

template <typename T>
inline std::vector<T> Mesh::gridSpacing() const
{
    return readVectorFloatingpoint<T>("gridSpacing");
}

template <typename T>
inline T Mesh::timeOffset() const
{
    return readFloatingpoint<T>("timeOffset");
}

std::ostream &operator<<(std::ostream &, openPMD::Mesh::Geometry const &);

std::ostream &operator<<(std::ostream &, openPMD::Mesh::DataOrder const &);

} // namespace openPMD
