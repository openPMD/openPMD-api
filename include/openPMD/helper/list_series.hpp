/* Copyright 2019-2021 Axel Huebl
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

#include "openPMD/Series.hpp"

#include <iostream>
#include <ostream>

namespace openPMD
{
namespace helper
{
    /** List information about an openPMD data series
     *
     * @param series a openPMD data path as in Series::Series
     * @param longer write more information
     * @param out    an output stream to write textual information to
     * @return reference to out as output stream, e.g. to pass the stream on via
     * `operator<<`
     */
    std::ostream &listSeries(
        Series &series,
        bool const longer = false,
        std::ostream &out = std::cout);
} // namespace helper
} // namespace openPMD
