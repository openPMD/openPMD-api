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

#if openPMD_HAVE_ADIOS1
#   include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#   include <adios.h>
#   include <adios_mpi.h> /* includes a dummy version of mpi if -D_NOMPI */
#endif

#include <future>
#include <memory>
#include <string>
#if openPMD_HAVE_ADIOS1
#   include <unordered_map>
#   include <unordered_set>
#endif


namespace openPMD
{
#if openPMD_HAVE_ADIOS1
class ADIOS1IOHandler;

class ADIOS1IOHandlerImpl : public AbstractIOHandlerImpl
{
public:
    ADIOS1IOHandlerImpl(AbstractIOHandler*);
    virtual ~ADIOS1IOHandlerImpl();

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

protected:
    int64_t m_group;
    std::string m_groupName;
    std::unordered_map< Writable*, int64_t > m_variableIDs;
    std::unordered_map< Writable*, int64_t > m_fileDescriptors;
    std::unordered_set< int64_t > m_openFileDescriptors;
};  //ADIOS1IOHandlerImpl
#else
class ADIOS1IOHandlerImpl
{ };
#endif

class ADIOS1IOHandler : public AbstractIOHandler
{
    friend class ADIOS1IOHandlerImpl;

public:
    ADIOS1IOHandler(std::string const& path, AccessType);
    virtual ~ADIOS1IOHandler() override;

    std::future< void > flush() override;

private:
    std::unique_ptr< ADIOS1IOHandlerImpl > m_impl;
};  //ADIOS1IOHandler
} // openPMD
