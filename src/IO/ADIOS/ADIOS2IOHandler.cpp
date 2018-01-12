#include <IO/ADIOS/ADIOS2IOHandler.hpp>
#ifdef LIBOPENPMD_WITH_ADIOS2
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <adios2.h>

#include <boost/filesystem.hpp>

#include <Auxiliary.hpp>
#include <IO/ADIOS/ADIOS2FilePosition.hpp>


class ADIOS2IOHandlerImpl
{
public:
    ADIOS2IOHandlerImpl(ADIOS2IOHandler*);
    virtual ~ADIOS2IOHandlerImpl();

    std::future< void > flush();

    using ArgumentMap = std::map< std::string, ParameterArgument >;
    void createFile(Writable*, ArgumentMap const&);

    ADIOS2IOHandler* m_handler;
    std::unordered_map< Writable*, std::shared_ptr< adios2::Engine > > m_files;
    std::unordered_set< std::shared_ptr< adios2::Engine > > m_openFiles;

    adios2::ADIOS m_adiosFactory;
};  //ADIOS2IOHandlerImpl

ADIOS2IOHandler::ADIOS2IOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at),
          m_impl{new ADIOS2IOHandlerImpl(this)}
{ }

ADIOS2IOHandler::~ADIOS2IOHandler()
{ }

std::future< void >
ADIOS2IOHandler::flush()
{
    return m_impl->flush();
}

ADIOS2IOHandlerImpl::ADIOS2IOHandlerImpl(ADIOS2IOHandler* handler)
        : m_handler{handler},
          m_adiosFactory(true)
{ }

ADIOS2IOHandlerImpl::~ADIOS2IOHandlerImpl()
{
    for( auto& file : m_openFiles )
        if( file )
            file->Close();
}

std::future< void >
ADIOS2IOHandlerImpl::flush()
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
                std::cerr << "Not implemented in ADIOS2 backend yet\n";
                break;
        }
        m_handler->m_work.pop();
    }
    return std::future< void >();
}

void ADIOS2IOHandlerImpl::createFile(Writable* writable,
                                     ArgumentMap const& parameters)
{
    if( !writable->written )
    {
        using namespace boost::filesystem;
        path dir(m_handler->directory);
        if( !exists(dir) )
            create_directories(dir);

        /* Create a new file using MPI properties. */
        std::string name = m_handler->directory + parameters.at("name").get< std::string >();
        if( !ends_with(name, ".bp") )
            name += ".bp";
        adios2::IO &bpIO = m_adiosFactory.DeclareIO("BPFile_N2N");
        bpIO.SetEngine("BPFileWriter");
        auto bpWriter = bpIO.Open(name, adios2::OpenMode::Write);

        if (!bpWriter)
            throw std::runtime_error("Interal error: Failed to write ADIOS2 file");

        writable->written = true;
        writable->abstractFilePosition = std::make_shared< ADIOS2FilePosition >();

        m_files.insert({writable, bpWriter});
        m_openFiles.insert(bpWriter);
        while( (writable = writable->parent) )
            m_files.insert({writable, bpWriter});
    }
}
#else
class ADIOS2IOHandlerImpl
{ };

ADIOS2IOHandler::ADIOS2IOHandler(std::string const& path, AccessType at)
        : AbstractIOHandler(path, at)
{
    throw std::runtime_error("libopenPMD built without ADIOS2 support");
}

ADIOS2IOHandler::~ADIOS2IOHandler()
{ }

std::future< void >
ADIOS2IOHandler::flush()
{
    return std::future< void >();
}
#endif
