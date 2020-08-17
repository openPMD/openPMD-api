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
#include "openPMD/IO/AbstractIOHandlerHelper.hpp"

#include "openPMD/IO/ADIOS/ADIOS1IOHandler.hpp"
#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"
#include "openPMD/IO/ADIOS/ParallelADIOS1IOHandler.hpp"
#include "openPMD/IO/DummyIOHandler.hpp"
#include "openPMD/IO/HDF5/HDF5IOHandler.hpp"
#include "openPMD/IO/HDF5/ParallelHDF5IOHandler.hpp"
#include "openPMD/IO/JSON/JSONIOHandler.hpp"
#include <nlohmann/json.hpp>

namespace openPMD
{
#if openPMD_HAVE_MPI
    std::shared_ptr< AbstractIOHandler >
    createIOHandler(
        std::string path,
        Access access,
        Format format,
        MPI_Comm comm,
        std::string const & options )
    {
        nlohmann::json optionsJson = nlohmann::json::parse( options );
        switch( format )
        {
            case Format::HDF5:
                return std::make_shared< ParallelHDF5IOHandler >( path, access, comm );
            case Format::ADIOS1:
#   if openPMD_HAVE_ADIOS1
                return std::make_shared< ParallelADIOS1IOHandler >( path, access, comm );
#   else
                throw std::runtime_error("openPMD-api built without ADIOS1 support");
#   endif
            case Format::ADIOS2:
                return std::make_shared< ADIOS2IOHandler >(
                    path, access, comm, std::move( optionsJson ), "bp4" );
            case Format::ADIOS2_SST:
                return std::make_shared< ADIOS2IOHandler >(
                    path, access, comm, std::move( optionsJson ), "sst" );
            default:
                throw std::runtime_error(
                    "Unknown file format! Did you specify a file ending?" );
        }
    }
#endif

    std::shared_ptr< AbstractIOHandler >
    createIOHandler(
        std::string path,
        Access access,
        Format format,
        std::string const & options )
    {
        nlohmann::json optionsJson = nlohmann::json::parse( options );
        switch( format )
        {
            case Format::HDF5:
                return std::make_shared< HDF5IOHandler >( path, access );
            case Format::ADIOS1:
#if openPMD_HAVE_ADIOS1
                return std::make_shared< ADIOS1IOHandler >( path, access );
#else
                throw std::runtime_error("openPMD-api built without ADIOS1 support");
#endif
#if openPMD_HAVE_ADIOS2
            case Format::ADIOS2:
                return std::make_shared< ADIOS2IOHandler >(
                    path, access, std::move( optionsJson ), "bp4" );
            case Format::ADIOS2_SST:
                return std::make_shared< ADIOS2IOHandler >(
                    path, access, std::move( optionsJson ), "sst" );
#endif // openPMD_HAVE_ADIOS2
            case Format::JSON:
                return std::make_shared< JSONIOHandler >( path, access );
            default:
                throw std::runtime_error(
                    "Unknown file format! Did you specify a file ending?" );
        }
    }
    } // namespace openPMD
