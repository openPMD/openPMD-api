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
#if openPMD_HAVE_HDF5
#   include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#   include <hdf5.h>
#   include <unordered_map>
#   include <unordered_set>
#endif


namespace openPMD
{
#if openPMD_HAVE_HDF5
    class HDF5IOHandlerImpl : public AbstractIOHandlerImpl
    {
    public:
        HDF5IOHandlerImpl(AbstractIOHandler*);
        virtual ~HDF5IOHandlerImpl();

        void createFile(Writable*, Parameter< Operation::CREATE_FILE > const&) override;
        void createPath(Writable*, Parameter< Operation::CREATE_PATH > const&) override;
        void createDataset(Writable*, Parameter< Operation::CREATE_DATASET > const&) override;
        void extendDataset(Writable*, Parameter< Operation::EXTEND_DATASET > const&) override;
        void openFile(Writable*, Parameter< Operation::OPEN_FILE > const&) override;
        void openPath(Writable*, Parameter< Operation::OPEN_PATH > const&) override;
        void openDataset(Writable*, Parameter< Operation::OPEN_DATASET > &) override;
        void deleteFile(Writable*, Parameter< Operation::DELETE_FILE > const&) override;
        void deletePath(Writable*, Parameter< Operation::DELETE_PATH > const&) override;
        void deleteDataset(Writable*, Parameter< Operation::DELETE_DATASET > const&) override;
        void deleteAttribute(Writable*, Parameter< Operation::DELETE_ATT > const&) override;
        void writeDataset(Writable*, Parameter< Operation::WRITE_DATASET > const&) override;
        void writeAttribute(Writable*, Parameter< Operation::WRITE_ATT > const&) override;
        void readDataset(Writable*, Parameter< Operation::READ_DATASET > &) override;
        void readAttribute(Writable*, Parameter< Operation::READ_ATT > &) override;
        void listPaths(Writable*, Parameter< Operation::LIST_PATHS > &) override;
        void listDatasets(Writable*, Parameter< Operation::LIST_DATASETS > &) override;
        void listAttributes(Writable*, Parameter< Operation::LIST_ATTS > &) override;

        std::unordered_map< Writable*, hid_t > m_fileIDs;
        std::unordered_set< hid_t > m_openFileIDs;

        hid_t m_datasetTransferProperty;
        hid_t m_fileAccessProperty;

        hid_t m_H5T_BOOL_ENUM;
    }; // HDF5IOHandlerImpl
#else
    class HDF5IOHandlerImpl
    {
    }; // HDF5IOHandlerImpl
#endif

} // openPMD
