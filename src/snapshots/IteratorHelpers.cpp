/* Copyright 2025 Franz Poeschel
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
#include "openPMD/snapshots/IteratorHelpers.hpp"

namespace openPMD
{
using value_type =
    Container<Iteration, Iteration::IterationIndex_t>::value_type;
auto stateful_to_opaque(StatefulIterator const &it) -> OpaqueSeriesIterator<
    Container<Iteration, Iteration::IterationIndex_t>::value_type>
{
    return from_concrete_iterator<
        StatefulIterator,
        Container<Iteration, Iteration::IterationIndex_t>::value_type>(it);
}
} // namespace openPMD
