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

#include "openPMD/config.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"

#include <hdf5.h>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>


namespace openPMD
{
#if openPMD_HAVE_HDF5
    struct GetH5DataType
    {
        std::unordered_map< std::string, hid_t > m_userTypes;

        GetH5DataType( std::unordered_map< std::string, hid_t > userTypes )
        : m_userTypes{ std::move(userTypes) }
        {
        }

        hid_t
        operator()(Attribute const &att);
    };

    hid_t
    getH5DataSpace(Attribute const& att);

    std::string
    concrete_h5_file_position(Writable* w);

#endif
} // namespace openPMD
