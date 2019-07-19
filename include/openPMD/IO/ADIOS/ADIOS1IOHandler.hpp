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
    class EXPORT ADIOS1IOHandlerImpl;

#if openPMD_HAVE_ADIOS1
    class EXPORT ADIOS1IOHandler : public AbstractIOHandler
    {
        friend class ADIOS1IOHandlerImpl;

    public:
        ADIOS1IOHandler(std::string path, AccessType);
        ~ADIOS1IOHandler() override;

        std::future< void > flush() override;

        void enqueue(IOTask const&) override;

    private:
        std::queue< IOTask > m_setup;
        std::unique_ptr< ADIOS1IOHandlerImpl > m_impl;
    }; // ADIOS1IOHandler
#else
    class EXPORT ADIOS1IOHandler : public AbstractIOHandler
    {
        friend class ADIOS1IOHandlerImpl;

    public:
        ADIOS1IOHandler(std::string path, AccessType);
        ~ADIOS1IOHandler() override;

        std::future< void > flush() override;

    private:
        std::unique_ptr< ADIOS1IOHandlerImpl > m_impl;
    }; // ADIOS1IOHandler
#endif
} // openPMD

#undef EXPORT
