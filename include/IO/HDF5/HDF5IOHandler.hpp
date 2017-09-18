#pragma once


#include <memory>
#include <string>

#include <IO/AbstractIOHandler.hpp>

#ifdef LIBOPENPMD_WITH_HDF5
#include <unordered_map>
#include <unordered_set>

#include <hdf5.h>

class HDF5IOHandler;

class HDF5IOHandlerImpl
{
public:
    HDF5IOHandlerImpl(AbstractIOHandler*);
    virtual ~HDF5IOHandlerImpl();

    virtual std::future< void > flush();

    using ArgumentMap = std::map< std::string, Argument >;
    virtual void createFile(Writable*, ArgumentMap const&);
    virtual void createPath(Writable*, ArgumentMap const&);
    virtual void createDataset(Writable *, ArgumentMap const&);
    virtual void openFile(Writable*, ArgumentMap const&);
    virtual void openPath(Writable*, ArgumentMap const&);
    virtual void openDataset(Writable*, ArgumentMap &);
    virtual void deleteFile(Writable*, ArgumentMap const&);
    virtual void deletePath(Writable*, ArgumentMap const&);
    virtual void deleteDataset(Writable*, ArgumentMap const&);
    virtual void deleteAttribute(Writable*, ArgumentMap const&);
    virtual void writeDataset(Writable*, ArgumentMap const&);
    virtual void writeAttribute(Writable*, ArgumentMap const&);
    virtual void readDataset(Writable*, ArgumentMap &);
    virtual void readAttribute(Writable*, ArgumentMap &);
    virtual void listPaths(Writable*, ArgumentMap &);
    virtual void listDatasets(Writable*, ArgumentMap &);
    virtual void listAttributes(Writable*, ArgumentMap &);

    std::unordered_map< Writable*, hid_t > m_fileIDs;
    std::unordered_set< hid_t > m_openFileIDs;

    hid_t m_datasetTransferProperty;
    hid_t m_fileAccessProperty;

    hid_t m_H5T_BOOL_ENUM;

    AbstractIOHandler* m_handler;
};  //HDF5IOHandlerImpl
#else
class HDF5IOHandlerImpl
{ };
#endif

class HDF5IOHandler : public AbstractIOHandler
{
public:
    HDF5IOHandler(std::string const& path, AccessType);
    virtual ~HDF5IOHandler();

    std::future< void > flush() override;

private:
    std::unique_ptr< HDF5IOHandlerImpl > m_impl;
};  //HDF5IOHandler
