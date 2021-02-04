/* Copyright 2020-2021 Axel Huebl
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

namespace openPMD
{
namespace python
{
    constexpr auto doc_unit_dimension = R"docstr(
Return the physical dimension (quantity) of a record

Annotating the physical dimension of a record allows us to read data
sets with arbitrary names and understand their purpose simply by
dimensional analysis. The dimensional base quantities in openPMD are
in order: length (L), mass (M), time (T), electric current (I),
thermodynamic temperature (theta), amount of substance (N),
luminous intensity (J) after the international system of quantities
(ISQ).

See https://en.wikipedia.org/wiki/Dimensional_analysis
See https://en.wikipedia.org/wiki/International_System_of_Quantities#Base_quantities
See https://github.com/openPMD/openPMD-standard/blob/1.1.0/STANDARD.md#required-for-each-record

Returns the powers of the 7 base measures in the order specified above.
)docstr";

} // namespace python
} // namespace openPMD
