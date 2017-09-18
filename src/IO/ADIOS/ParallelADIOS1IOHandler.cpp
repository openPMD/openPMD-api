#include <IO/ADIOS/ParallelADIOS1IOHandler.hpp>
#ifdef LIBOPENPMD_WITH_PARALLEL_ADIOS1
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <adios.h>
#include <mpi.h>

#include <boost/filesystem.hpp>

#include <Auxiliary.hpp>
#include <IO/ADIOS/ADIOS1FilePosition.hpp>


class ParallelADIOS1IOHandlerImpl
{
public:
    ParallelADIOS1IOHandlerImpl(ParallelADIOS1IOHandler*);
    virtual ~ParallelADIOS1IOHandlerImpl();

    std::future< void > flush();

    using ArgumentMap = std::map< std::string, Argument >;
    void createFile(Writable*, ArgumentMap const&);

    MPI_Comm m_mpiComm;
    MPI_Info m_mpiInfo;

    ParallelADIOS1IOHandler* m_handler;
    std::unordered_map< Writable*, int64_t > m_files;
    std::unordered_set< int64_t > m_openFiles;
    int64_t m_groupID;
};  //ParallelADIOS1IOHandlerImpl

ParallelADIOS1IOHandler::ParallelADIOS1IOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at),
          m_impl{new ParallelADIOS1IOHandlerImpl(this)}
{ }

ParallelADIOS1IOHandler::~ParallelADIOS1IOHandler()
{ }

std::future< void >
ParallelADIOS1IOHandler::flush()
{
    return m_impl->flush();
}

ParallelADIOS1IOHandlerImpl::ParallelADIOS1IOHandlerImpl(ParallelADIOS1IOHandler* handler)
        : m_mpiComm{MPI_COMM_WORLD},
          m_mpiInfo{MPI_INFO_NULL},
          m_handler{handler}
{
    adios_init_noxml(m_mpiComm);
    std::string groupName{""};
    std::string timeIndex{""};
    adios_declare_group(&m_groupID,
                        groupName.c_str(),
                        timeIndex.c_str(),
                        ADIOS_STATISTICS_FLAG::adios_stat_default);
}

ParallelADIOS1IOHandlerImpl::~ParallelADIOS1IOHandlerImpl()
{
    for( auto const& file : m_openFiles )
        adios_close(file);

    adios_free_group(m_groupID);

    MPI_Barrier(m_mpiComm);

    int mpiRank{-1};
    MPI_Comm_rank(m_mpiComm, &mpiRank);
    adios_finalize(mpiRank);
}

std::future< void >
ParallelADIOS1IOHandlerImpl::flush()
{
    while( !(m_handler->m_work.empty()) )
    {
        IOTask& i = m_handler->m_work.front();
        switch( i.operation )
        {
            using O = Operation;
            case O::CREATE_FILE:
                createFile(i.writable, i.parameter);
                break;
            case O::CREATE_PATH:
                //createPath(i.writable, i.parameter);
                //break;
            case O::CREATE_DATASET:
                //createDataset(i.writable, i.parameter);
                //break;
            case O::OPEN_FILE:
                //openFile(i.writable, i.parameter);
                //break;
            case O::OPEN_PATH:
                //openPath(i.writable, i.parameter);
                //break;
            case O::OPEN_DATASET:
                //openDataset(i.writable, i.parameter);
                //break;
            case O::DELETE_FILE:
                //deleteFile(i.writable, i.parameter);
                //break;
            case O::DELETE_PATH:
                //deletePath(i.writable, i.parameter);
                //break;
            case O::DELETE_DATASET:
                //deleteDataset(i.writable, i.parameter);
                //break;
            case O::DELETE_ATT:
                //deleteAttribute(i.writable, i.parameter);
                //break;
            case O::WRITE_DATASET:
                //writeDataset(i.writable, i.parameter);
                //break;
            case O::WRITE_ATT:
                //writeAttribute(i.writable, i.parameter);
                //break;
            case O::READ_DATASET:
                //readDataset(i.writable, i.parameter);
                //break;
            case O::READ_ATT:
                //readAttribute(i.writable, i.parameter);
                //break;
            case O::LIST_PATHS:
                //listPaths(i.writable, i.parameter);
                //break;
            case O::LIST_DATASETS:
                //listDatasets(i.writable, i.parameter);
                //break;
            case O::LIST_ATTS:
                //listAttributes(i.writable, i.parameter);
                std::cerr << "Not implemented in ParallelADIOS1 backend yet\n";
                break;
        }
        m_handler->m_work.pop();
    }
    return std::future< void >();
}

void ParallelADIOS1IOHandlerImpl::createFile(Writable* writable,
                                     ArgumentMap const& parameters)
{
    if( !writable->written )
    {
        using namespace boost::filesystem;
        path dir(m_handler->directory);
        if( !exists(dir) )
            create_directories(dir);

        /* Create a new file. */
        std::string name = m_handler->directory + parameters.at("name").get< std::string >();
        if( !ends_with(name, ".bp") )
            name += ".bp";
        int64_t file;
        adios_open(&file, "", name.c_str(), "w", m_mpiComm);

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< ADIOS1FilePosition >();

        m_files.insert({writable, file});
        m_openFiles.insert(file);
        while( (writable = writable->parent) )
            m_files.insert({writable, file});
    }
}
#else
class ParallelADIOS1IOHandlerImpl
{ };

ParallelADIOS1IOHandler::ParallelADIOS1IOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at)
{
    throw std::runtime_error("libopenPMD built without ADIOS support");
}

ParallelADIOS1IOHandler::~ParallelADIOS1IOHandler()
{ }

std::future< void >
ParallelADIOS1IOHandler::flush()
{
    return std::future< void >();
}
#endif