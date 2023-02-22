/* Copyright 2017-2021 Fabian Koller and Franz Poeschel
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

#if openPMD_HAVE_ADIOS1

#include "openPMD/IO/ADIOS/ADIOS1Auxiliary.hpp"
#include "openPMD/IO/ADIOS/ADIOS1FilePosition.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/StringManip.hpp"

#include <adios.h>
#include <adios_read.h>

#include <future>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace openPMD
{
template <typename ChildClass> // CRT pattern
class CommonADIOS1IOHandlerImpl : public AbstractIOHandlerImpl
{
public:
    void
    createFile(Writable *, Parameter<Operation::CREATE_FILE> const &) override;
    void checkFile(Writable *, Parameter<Operation::CHECK_FILE> &) override;
    void
    createPath(Writable *, Parameter<Operation::CREATE_PATH> const &) override;
    void createDataset(
        Writable *, Parameter<Operation::CREATE_DATASET> const &) override;
    void extendDataset(
        Writable *, Parameter<Operation::EXTEND_DATASET> const &) override;
    void openFile(Writable *, Parameter<Operation::OPEN_FILE> &) override;
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
    void
    writeDataset(Writable *, Parameter<Operation::WRITE_DATASET> &) override;
    void writeAttribute(
        Writable *, Parameter<Operation::WRITE_ATT> const &) override;
    void readDataset(Writable *, Parameter<Operation::READ_DATASET> &) override;
    void readAttribute(Writable *, Parameter<Operation::READ_ATT> &) override;
    void listPaths(Writable *, Parameter<Operation::LIST_PATHS> &) override;
    void
    listDatasets(Writable *, Parameter<Operation::LIST_DATASETS> &) override;
    void listAttributes(Writable *, Parameter<Operation::LIST_ATTS> &) override;
    void
    deregister(Writable *, Parameter<Operation::DEREGISTER> const &) override;

    void close(int64_t);
    void close(ADIOS_FILE *);
    void
    flush_attribute(int64_t group, std::string const &name, Attribute const &);

protected:
    template <typename... Args>
    CommonADIOS1IOHandlerImpl(Args &&...args)
        : AbstractIOHandlerImpl{std::forward<Args>(args)...}
    {}

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
    // config options
    std::string m_defaultTransform;
    /**
     * Call this function to get adios file id for a Writable. Will create one
     * if does not exist
     * @return  returns an adios file id.
     */
    int64_t GetFileHandle(Writable *);

    void initJson(json::TracingJSON);
}; // ParallelADIOS1IOHandlerImpl
} // namespace openPMD

#endif
