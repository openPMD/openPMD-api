/* Copyright 2017-2019 Franz Pöschel
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

#include "openPMD/IO/JSON/JSONIOHandler.hpp"


namespace openPMD
{
    JSONIOHandler::~JSONIOHandler( )
    {}


    JSONIOHandler::JSONIOHandler(
        std::string path,
        AccessType at
    ) :
        AbstractIOHandler {
            path,
            at
        },
        m_impl { JSONIOHandlerImpl { this } }
    {}

    std::future< void > JSONIOHandler::flush( )
    {
        return m_impl.flush( );
    }
} // openPMD
