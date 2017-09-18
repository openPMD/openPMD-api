#pragma once

#include <IO/AbstractIOHandler.hpp>

#ifdef LIBOPENPMD_WITH_PARALLEL_HDF5
#include <IO/HDF5/HDF5IOHandler.hpp>

class ParallelHDF5IOHandler;

class ParallelHDF5IOHandlerImpl : public HDF5IOHandlerImpl
{
public:
    ParallelHDF5IOHandlerImpl(AbstractIOHandler*);
    virtual ~ParallelHDF5IOHandlerImpl();

    MPI_Comm m_mpiComm;
    MPI_Info m_mpiInfo;
};  //ParallelHDF5IOHandlerImpl
#else
class ParallelHDF5IOHandlerImpl
{ };
#endif

class ParallelHDF5IOHandler : public AbstractIOHandler
{
public:
    ParallelHDF5IOHandler(std::string const& path, AccessType);
    virtual ~ParallelHDF5IOHandler();

    std::future< void > flush() override;

private:
    std::unique_ptr< ParallelHDF5IOHandlerImpl > m_impl;
};  //ParallelHDF5IOHandler
