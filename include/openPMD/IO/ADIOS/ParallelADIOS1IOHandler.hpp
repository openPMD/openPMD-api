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
#pragma once

#include "openPMD/config.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"

#include <future>
#include <memory>
#include <string>
#if openPMD_HAVE_ADIOS1
#   include <queue>
#endif

#if _MSC_VER
#   define EXPORT __declspec( dllexport )
#else
#   define EXPORT __attribute__((visibility("default")))
#endif


namespace openPMD
{
    class EXPORT ParallelADIOS1IOHandlerImpl;

    class EXPORT ParallelADIOS1IOHandler : public AbstractIOHandler
    {
        friend class ParallelADIOS1IOHandlerImpl;

    public:
#   if openPMD_HAVE_MPI
        ParallelADIOS1IOHandler(std::string path, AccessType, MPI_Comm);
#   else
        ParallelADIOS1IOHandler(std::string path, AccessType);
#   endif
        ~ParallelADIOS1IOHandler() override;

        std::future< void > flush() override;
#if openPMD_HAVE_ADIOS1
        void enqueue(IOTask const&) override;
#endif

    private:
#if openPMD_HAVE_ADIOS1
        std::queue< IOTask > m_setup;
#endif
        std::unique_ptr< ParallelADIOS1IOHandlerImpl > m_impl;
    }; // ParallelADIOS1IOHandler

} // openPMD

#undef EXPORT
