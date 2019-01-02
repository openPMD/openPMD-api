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

#include "openPMD/IO/AbstractIOHandler.hpp"

#include <future>
#include <memory>
#include <string>


namespace openPMD
{
#if openPMD_HAVE_ADIOS2
class ADIOS2IOHandler;

class ADIOS2IOHandlerImpl
{
public:
    ADIOS2IOHandlerImpl(AbstractIOHandler*);
    virtual ~ADIOS2IOHandlerImpl();

    virtual std::future< void > flush();

    using ArgumentMap = std::map< std::string, ParameterArgument >;
    virtual void createFile(Writable*, ArgumentMap const&);
    virtual void createPath(Writable*, ArgumentMap const&);
    virtual void createDataset(Writable*, ArgumentMap const&);
    virtual void extendDataset(Writable*, ArgumentMap const&);
    virtual void openFile(Writable*, ArgumentMap const&);
    virtual void openPath(Writable*, ArgumentMap const&);
    virtual void openDataset(Writable*, ArgumentMap &);
    virtual void deleteFile(Writable*, ArgumentMap const&);
    virtual void deletePath(Writable*, ArgumentMap const&);
    virtual void deleteDataset(Writable*, ArgumentMap const&);
    virtual void deleteAttribute(Writable*, ArgumentMap const&);
    virtual void writeDataset(Writable*, ArgumentMap const&);
    virtual void writeAttribute(Writable*, ArgumentMap const&);
    virtual void readDataset(Writable*, ArgumentMap &);
    virtual void readAttribute(Writable*, ArgumentMap &);
    virtual void listPaths(Writable*, ArgumentMap &);
    virtual void listDatasets(Writable*, ArgumentMap &);
    virtual void listAttributes(Writable*, ArgumentMap &);

    AbstractIOHandler* m_handler;
};  //ADIOS2IOHandlerImpl
#else
class ADIOS2IOHandlerImpl
{ };
#endif

class ADIOS2IOHandler : public AbstractIOHandler
{
    friend class ADIOS2IOHandlerImpl;

public:
    ADIOS2IOHandler(std::string path, AccessType);
    ~ADIOS2IOHandler() override;

    std::future< void > flush() override;

private:
    std::unique_ptr< ADIOS2IOHandlerImpl > m_impl;
};  //ADIOS2IOHandler
} // openPMD
