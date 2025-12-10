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
#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"

namespace openPMD
{
auto stateful_to_opaque(StatefulIterator const &it) -> OpaqueSeriesIterator<
    Container<Iteration, Iteration::IterationIndex_t>::value_type>;

template <
    typename ConcreteIteratorClass,
    typename ValueType,
    typename... ConstructorArgs>
auto from_concrete_iterator(ConstructorArgs &&...args)
    -> OpaqueSeriesIterator<ValueType>
{
    return OpaqueSeriesIterator<ValueType>(
        std::unique_ptr<DynamicSeriesIterator<ValueType>>{
            new ConcreteIteratorClass(std::forward<ConstructorArgs>(args)...)});
}
} // namespace openPMD
