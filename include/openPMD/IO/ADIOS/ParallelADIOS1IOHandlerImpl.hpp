/* Copyright 2017-2021 Fabian Koller
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
#include "openPMD/auxiliary/Export.hpp"
#include "openPMD/config.hpp"

#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include <adios.h>
#include <adios_read.h>
#endif

#include <future>
#include <memory>
#include <string>
#if openPMD_HAVE_ADIOS1
#include <unordered_map>
#include <unordered_set>
#endif

namespace openPMD
{
#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
class OPENPMDAPI_EXPORT ParallelADIOS1IOHandlerImpl
    : public AbstractIOHandlerImpl
{
public:
    ParallelADIOS1IOHandlerImpl(AbstractIOHandler *, MPI_Comm);
    virtual ~ParallelADIOS1IOHandlerImpl();

    virtual void init();

    std::future<void> flush();

    void
    createFile(Writable *, Parameter<Operation::CREATE_FILE> const &) override;
    void
    createPath(Writable *, Parameter<Operation::CREATE_PATH> const &) override;
    void createDataset(
        Writable *, Parameter<Operation::CREATE_DATASET> const &) override;
    void extendDataset(
        Writable *, Parameter<Operation::EXTEND_DATASET> const &) override;
    void openFile(Writable *, Parameter<Operation::OPEN_FILE> const &) override;
    void
    closeFile(Writable *, Parameter<Operation::CLOSE_FILE> const &) override;
    void availableChunks(
        Writable *, Parameter<Operation::AVAILABLE_CHUNKS> &) override;
    void openPath(Writable *, Parameter<Operation::OPEN_PATH> const &) override;
    void openDataset(Writable *, Parameter<Operation::OPEN_DATASET> &) override;
    void
    deleteFile(Writable *, Parameter<Operation::DELETE_FILE> const &) override;
    void
    deletePath(Writable *, Parameter<Operation::DELETE_PATH> const &) override;
    void deleteDataset(
        Writable *, Parameter<Operation::DELETE_DATASET> const &) override;
    void deleteAttribute(
        Writable *, Parameter<Operation::DELETE_ATT> const &) override;
    void writeDataset(
        Writable *, Parameter<Operation::WRITE_DATASET> const &) override;
    void writeAttribute(
        Writable *, Parameter<Operation::WRITE_ATT> const &) override;
    void readDataset(Writable *, Parameter<Operation::READ_DATASET> &) override;
    void readAttribute(Writable *, Parameter<Operation::READ_ATT> &) override;
    void listPaths(Writable *, Parameter<Operation::LIST_PATHS> &) override;
    void
    listDatasets(Writable *, Parameter<Operation::LIST_DATASETS> &) override;
    void listAttributes(Writable *, Parameter<Operation::LIST_ATTS> &) override;

    virtual int64_t open_write(Writable *);
    virtual ADIOS_FILE *open_read(std::string const &name);
    void close(int64_t);
    void close(ADIOS_FILE *);
    int64_t initialize_group(std::string const &name);
    void
    flush_attribute(int64_t group, std::string const &name, Attribute const &);

protected:
    ADIOS_READ_METHOD m_readMethod;
    std::unordered_map<Writable *, std::shared_ptr<std::string> > m_filePaths;
    std::unordered_map<std::shared_ptr<std::string>, int64_t> m_groups;
    std::unordered_map<std::shared_ptr<std::string>, bool> m_existsOnDisk;
    std::unordered_map<std::shared_ptr<std::string>, int64_t>
        m_openWriteFileHandles;
    std::unordered_map<std::shared_ptr<std::string>, ADIOS_FILE *>
        m_openReadFileHandles;
    struct ScheduledRead
    {
        ADIOS_SELECTION *selection;
        std::shared_ptr<void> data; // needed to avoid early freeing
    };
    std::unordered_map<ADIOS_FILE *, std::vector<ScheduledRead> >
        m_scheduledReads;
    std::unordered_map<int64_t, std::unordered_map<std::string, Attribute> >
        m_attributeWrites;
    /**
     * Call this function to get adios file id for a Writable. Will create one
     * if does not exist
     * @return  returns an adios file id.
     */
    int64_t GetFileHandle(Writable *);
    MPI_Comm m_mpiComm;
    MPI_Info m_mpiInfo;
}; // ParallelADIOS1IOHandlerImpl
#else
class OPENPMDAPI_EXPORT ParallelADIOS1IOHandlerImpl
{}; // ParallelADIOS1IOHandlerImpl
#endif

} // namespace openPMD
