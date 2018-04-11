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

#include "openPMD/IO/AbstractIOHandler.hpp"

#if openPMD_HAVE_HDF5
#   include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#   include <hdf5.h>
#endif

#include <future>
#include <memory>
#include <string>
#if openPMD_HAVE_HDF5
#   include <unordered_map>
#   include <unordered_set>
#endif


namespace openPMD
{
#if openPMD_HAVE_HDF5
class HDF5IOHandler;

class HDF5IOHandlerImpl : public AbstractIOHandlerImpl
{
public:
    HDF5IOHandlerImpl(AbstractIOHandler*);
    virtual ~HDF5IOHandlerImpl();

    virtual void createFile(Writable*, Parameter< Operation::CREATE_FILE > const&) override;
    virtual void createPath(Writable*, Parameter< Operation::CREATE_PATH > const&) override;
    virtual void createDataset(Writable*, Parameter< Operation::CREATE_DATASET > const&) override;
    virtual void extendDataset(Writable*, Parameter< Operation::EXTEND_DATASET > const&) override;
    virtual void openFile(Writable*, Parameter< Operation::OPEN_FILE > const&) override;
    virtual void openPath(Writable*, Parameter< Operation::OPEN_PATH > const&) override;
    virtual void openDataset(Writable*, Parameter< Operation::OPEN_DATASET > &) override;
    virtual void deleteFile(Writable*, Parameter< Operation::DELETE_FILE > const&) override;
    virtual void deletePath(Writable*, Parameter< Operation::DELETE_PATH > const&) override;
    virtual void deleteDataset(Writable*, Parameter< Operation::DELETE_DATASET > const&) override;
    virtual void deleteAttribute(Writable*, Parameter< Operation::DELETE_ATT > const&) override;
    virtual void writeDataset(Writable*, Parameter< Operation::WRITE_DATASET > const&) override;
    virtual void writeAttribute(Writable*, Parameter< Operation::WRITE_ATT > const&) override;
    virtual void readDataset(Writable*, Parameter< Operation::READ_DATASET > &) override;
    virtual void readAttribute(Writable*, Parameter< Operation::READ_ATT > &) override;
    virtual void listPaths(Writable*, Parameter< Operation::LIST_PATHS > &) override;
    virtual void listDatasets(Writable*, Parameter< Operation::LIST_DATASETS > &) override;
    virtual void listAttributes(Writable*, Parameter< Operation::LIST_ATTS > &) override;

    std::unordered_map< Writable*, hid_t > m_fileIDs;
    std::unordered_set< hid_t > m_openFileIDs;

    hid_t m_datasetTransferProperty;
    hid_t m_fileAccessProperty;

    hid_t m_H5T_BOOL_ENUM;
};  //HDF5IOHandlerImpl
#else
class HDF5IOHandlerImpl
{ };    //HDF5IOHandlerImpl
#endif

class HDF5IOHandler : public AbstractIOHandler
{
public:
    HDF5IOHandler(std::string const& path, AccessType);
    virtual ~HDF5IOHandler() override;

    std::future< void > flush() override;

private:
    std::unique_ptr< HDF5IOHandlerImpl > m_impl;
};  //HDF5IOHandler
} // openPMD
