/* Copyright 2017-2018 Fabian Koller
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

#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/Datatype.hpp"

#include <array>
#include <cstdint>
#include <vector>
#include <string>


namespace openPMD
{
//TODO This might have to be a Writable
//Reasoning - Flushes are expeted to be done often.
//Attributes should not be written unless dirty.
//At the moment the dirty check is done at Attributable level,
//resulting in all of an Attributables Attributes being written to disk even if only one changes
/** Varidic datatype supporting at least all formats for attributes specified in the openPMD standard.
 *
 * @note Extending and/or modifying the available formats requires identical
 *       modifications to Datatype.
 */
using Attribute = auxiliary::Variant< Datatype,
                            char, unsigned char,
                            int16_t, int32_t, int64_t,
                            uint16_t, uint32_t, uint64_t,
                            float, double, long double,
                            std::string,
                            std::vector< char >,
                            std::vector< int16_t >,
                            std::vector< int32_t >,
                            std::vector< int64_t >,
                            std::vector< unsigned char >,
                            std::vector< uint16_t >,
                            std::vector< uint32_t >,
                            std::vector< uint64_t >,
                            std::vector< float >,
                            std::vector< double >,
                            std::vector< long double >,
                            std::vector< std::string >,
                            std::array< double, 7 >,
                            bool >;
} // openPMD
