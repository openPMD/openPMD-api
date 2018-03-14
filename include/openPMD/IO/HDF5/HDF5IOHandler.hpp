/* Copyright 2017 Fabian Koller
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

#if openPMD_HAVE_HDF5
#   include <hdf5.h>
#endif

#include <future>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>


namespace openPMD
{
#if openPMD_HAVE_HDF5
class HDF5IOHandler;

class HDF5IOHandlerImpl
{
public:
    HDF5IOHandlerImpl(AbstractIOHandler*);
    virtual ~HDF5IOHandlerImpl();

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

    std::unordered_map< Writable*, hid_t > m_fileIDs;
    std::unordered_set< hid_t > m_openFileIDs;

    hid_t m_datasetTransferProperty;
    hid_t m_fileAccessProperty;

    hid_t m_H5T_BOOL_ENUM;

    AbstractIOHandler* m_handler;
};  //HDF5IOHandlerImpl
#else
class HDF5IOHandlerImpl
{ };
#endif

class HDF5IOHandler : public AbstractIOHandler
{
public:
    HDF5IOHandler(std::string const& path, AccessType);
    virtual ~HDF5IOHandler();

    std::future< void > flush() override;

private:
    std::unique_ptr< HDF5IOHandlerImpl > m_impl;
};  //HDF5IOHandler
} // openPMD
