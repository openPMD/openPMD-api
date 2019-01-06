/* Copyright 2017-2019 Fabian Koller
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
#include "openPMD/IO/DummyIOHandler.hpp"
#include "openPMD/IO/ADIOS/ADIOS1IOHandler.hpp"
#include "openPMD/IO/ADIOS/ParallelADIOS1IOHandler.hpp"
#include "openPMD/IO/HDF5/HDF5IOHandler.hpp"
#include "openPMD/IO/HDF5/ParallelHDF5IOHandler.hpp"
#include "openPMD/IO/JSON/JSONIOHandler.hpp"


namespace openPMD
{
#if openPMD_HAVE_MPI
    std::shared_ptr< AbstractIOHandler >
    createIOHandler(
        std::string path,
        AccessType accessType,
        Format format,
        MPI_Comm comm
    )
    {
        switch( format )
        {
            case Format::HDF5:
                return std::make_shared< ParallelHDF5IOHandler >(path, accessType, comm);
            case Format::ADIOS1:
                return std::make_shared< ParallelADIOS1IOHandler >(path, accessType, comm);
            case Format::ADIOS2:
                throw std::runtime_error("ADIOS2 backend not yet implemented");
            default:
                return std::make_shared< DummyIOHandler >(path, accessType);
        }
    }
#endif
    std::shared_ptr< AbstractIOHandler >
    createIOHandler(
        std::string path,
        AccessType accessType,
        Format format
    )
    {
        switch( format )
        {
            case Format::HDF5:
                return std::make_shared< HDF5IOHandler >(path, accessType);
            case Format::ADIOS1:
                return std::make_shared< ADIOS1IOHandler >(path, accessType);
            case Format::ADIOS2:
                throw std::runtime_error("ADIOS2 backend not yet implemented");
            case Format::JSON:
                return std::make_shared< JSONIOHandler >(path, accessType);
            default:
                return std::make_shared< DummyIOHandler >(path, accessType);
        }
    }
} // openPMD
