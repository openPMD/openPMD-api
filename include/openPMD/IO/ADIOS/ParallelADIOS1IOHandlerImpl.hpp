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

#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
#   include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#   include <adios.h>
#   include <adios_read.h>
#endif

#include <future>
#include <memory>
#include <string>
#if openPMD_HAVE_ADIOS1
#   include <unordered_map>
#   include <unordered_set>
#endif

#if _MSC_VER
#   define EXPORT __declspec( dllexport )
#else
#   define EXPORT __attribute__((visibility("default")))
#endif


namespace openPMD
{
#if openPMD_HAVE_ADIOS1 && openPMD_HAVE_MPI
    class EXPORT ParallelADIOS1IOHandlerImpl : public AbstractIOHandlerImpl
    {
    public:
        ParallelADIOS1IOHandlerImpl(AbstractIOHandler*, MPI_Comm);
        virtual ~ParallelADIOS1IOHandlerImpl();

        virtual void init();

        virtual std::future< void > flush() override;

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

        virtual int64_t open_write(Writable *);
        virtual ADIOS_FILE* open_read(std::string const& name);
        void close(int64_t);
        void close(ADIOS_FILE*);

    protected:
        int64_t m_group;
        std::string m_groupName;
        ADIOS_READ_METHOD m_readMethod;
        std::unordered_map< Writable*, std::shared_ptr< std::string > > m_filePaths;
        std::unordered_map< std::shared_ptr< std::string >, bool > m_existsOnDisk;
        std::unordered_map< std::shared_ptr< std::string >, int64_t > m_openWriteFileHandles;
        std::unordered_map< std::shared_ptr< std::string >, ADIOS_FILE* > m_openReadFileHandles;
        std::unordered_map< ADIOS_FILE*, std::vector< ADIOS_SELECTION* > > m_scheduledReads;
        MPI_Comm m_mpiComm;
        MPI_Info m_mpiInfo;
    }; // ParallelADIOS1IOHandlerImpl
#else
    class EXPORT ParallelADIOS1IOHandlerImpl
    {
    }; // ParallelADIOS1IOHandlerImpl
#endif

} // openPMD

#undef EXPORT
