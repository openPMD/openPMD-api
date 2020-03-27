/* Copyright 2017-2020 Fabian Koller, Axel Huebl
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
#include "openPMD/MPISeries.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>


namespace openPMD
{
    MPISeries::MPISeries(
        std::string const& filepath,
        AccessType at,
        MPI_Comm comm
    )
    {
        iterations = Container< Iteration, uint64_t >();
        m_iterationEncoding = std::make_shared< IterationEncoding >();

        auto input = parseInput(filepath);
        auto handler = createIOHandler(input->path, at, input->format, comm);
        init(handler, std::move(input));
    }

    MPISeries::~MPISeries()
    {
        // we must not throw in a destructor
        try
        {
            flush();
        }
        catch( std::exception const & ex )
        {
            std::cerr << "[~MPISeries] An error occurred: " << ex.what() << std::endl;
        }
        catch( ... )
        {
            std::cerr << "[~MPISeries] An error occurred." << std::endl;
        }
    }
} // namespace openPMD
