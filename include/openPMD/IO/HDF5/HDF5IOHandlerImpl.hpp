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

#include "openPMD/config.hpp"
#if openPMD_HAVE_HDF5
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"

#include "openPMD/auxiliary/JSON.hpp"
#include "openPMD/auxiliary/Option.hpp"

#include <hdf5.h>
#include <unordered_map>
#include <unordered_set>
#endif

namespace openPMD
{
#if openPMD_HAVE_HDF5
class HDF5IOHandlerImpl : public AbstractIOHandlerImpl
{
public:
    HDF5IOHandlerImpl(AbstractIOHandler *, nlohmann::json config);
    ~HDF5IOHandlerImpl() override;

    void
    createFile(Writable *, Parameter<Operation::CREATE_FILE> const &) override;
    void
    createPath(Writable *, Parameter<Operation::CREATE_PATH> const &) override;
    void createDataset(
        Writable *, Parameter<Operation::CREATE_DATASET> const &) override;
    void extendDataset(
        Writable *, Parameter<Operation::EXTEND_DATASET> const &) override;
    void availableChunks(
        Writable *, Parameter<Operation::AVAILABLE_CHUNKS> &) override;
    void openFile(Writable *, Parameter<Operation::OPEN_FILE> const &) override;
    void
    closeFile(Writable *, Parameter<Operation::CLOSE_FILE> const &) override;
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

    std::unordered_map<Writable *, std::string> m_fileNames;
    std::unordered_map<std::string, hid_t> m_fileNamesWithID;

    std::unordered_set<hid_t> m_openFileIDs;

    hid_t m_datasetTransferProperty;
    hid_t m_fileAccessProperty;
    hid_t m_fileCreateProperty;

    hbool_t m_hdf5_collective_metadata = 1;

    // h5py compatible types for bool and complex
    hid_t m_H5T_BOOL_ENUM;
    hid_t m_H5T_CFLOAT;
    hid_t m_H5T_CDOUBLE;
    hid_t m_H5T_CLONG_DOUBLE;

private:
    auxiliary::TracingJSON m_config;
    std::string m_chunks = "auto";
    struct File
    {
        std::string name;
        hid_t id;
    };
    auxiliary::Option<File> getFile(Writable *);
}; // HDF5IOHandlerImpl
#else
class HDF5IOHandlerImpl
{}; // HDF5IOHandlerImpl
#endif

} // namespace openPMD
