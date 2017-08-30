#include "../../../include/IO/HDF5/ParallelHDF5IOHandler.hpp"

#ifdef LIBOPENPMD_WITH_PARALLEL_HDF5
#include <unordered_map>
#include <unordered_set>

#include <mpi.h>

#include "../../../include/IO/HDF5/HDF5FilePosition.hpp"


class ParallelHDF5IOHandlerImpl
{
    friend class ParallelHDF5IOHandler;

public:
    ParallelHDF5IOHandlerImpl(ParallelHDF5IOHandler*);
    virtual ~ParallelHDF5IOHandlerImpl();

    std::future< void > flush();

    std::string concrete_file_position(Writable* w);
    void createFile(Writable*,
                    std::map< std::string, Argument > const&);
    void createPath(Writable*,
                    std::map< std::string, Argument > const&);
    void createDataset(Writable*,
                       std::map< std::string, Argument > const&);
    void openFile(Writable*,
                  std::map< std::string, Argument > const&);
    void openPath(Writable*,
                  std::map< std::string, Argument > const&);
    void openDataset(Writable*,
                     std::map< std::string, Argument >&);
    void writeDataset(Writable*,
                      std::map< std::string, Argument > const&);
    void writeAttribute(Writable*,
                        std::map< std::string, Argument > const&);
    void readDataset(Writable*,
                     std::map< std::string, Argument >&);
    void readAttribute(Writable*,
                       std::map< std::string, Argument >&);
    void listPaths(Writable*,
                   std::map< std::string, Argument >&);
    void listDatasets(Writable*,
                      std::map< std::string, Argument >&);
    void listAttributes(Writable*,
                        std::map< std::string, Argument >&);

    ParallelHDF5IOHandler* m_handler;
    std::unordered_map< Writable*, hid_t > m_fileIDs;
    std::unordered_set< hid_t > m_openFileIDs;
};  //ParallelHDF5IOHandlerImpl

ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string const& path,
                                             AccessType at)
        : AbstractIOHandler(path, at)
{ }

ParallelHDF5IOHandler::~ParallelHDF5IOHandler()
{ }

std::future< void >
ParallelHDF5IOHandler::flush()
{
    return m_impl->flush();
}

ParallelHDF5IOHandlerImpl::ParallelHDF5IOHandlerImpl(ParallelHDF5IOHandler* handler)
        : m_handler{handler}
{
    MPI_Comm comm  = MPI_COMM_WORLD;
    MPI_Info info  = MPI_INFO_NULL;

    hid_t plist_id = H5Pcreate(H5P_FILE_ACCESS);
    H5Pset_fapl_mpio(plist_id, comm, info);
}

ParallelHDF5IOHandlerImpl::~ParallelHDF5IOHandlerImpl()
{ }

std::future< void >
ParallelHDF5IOHandlerImpl::flush()
{
    return std::future< void >();
}

#else
class ParallelHDF5IOHandlerImpl
{ };

ParallelHDF5IOHandler::ParallelHDF5IOHandler(std::string const& path,
                                             AccessType at)
        : AbstractIOHandler(path, at)
{
    static_assert(false, "libopenPMD build without parallel HDF5 support");
}
#endif
