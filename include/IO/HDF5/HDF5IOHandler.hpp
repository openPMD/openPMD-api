#pragma once


#include <string>
#include <unordered_map>
#include <unordered_set>

#include "../AbstractIOHandler.hpp"
#include "../HDF5/HDF5FilePosition.hpp"
#include "../../Writable.hpp"


class HDF5IOHandler : public AbstractIOHandler
{
public:
    HDF5IOHandler(std::string const& path, AccessType);
    virtual ~HDF5IOHandler();

    std::future< void > flush();

private:
    std::unordered_map< Writable*, hid_t > m_fileIDs;
    std::unordered_set< hid_t > m_openFileIDs;
    std::string concrete_file_position(Writable *w);
    void createFile(Writable*,
                    std::map< std::string, Argument > const&);
    void createPath(Writable*,
                    std::map< std::string, Argument > const&);
    void createDataset(Writable *,
                       std::map< std::string, Argument > const&);
    void openFile(Writable*,
                  std::map< std::string, Argument > const&);
    void openPath(Writable*,
                  std::map< std::string, Argument > const&);
    void openDataset(Writable*,
                     std::map< std::string, Argument > &);
    void writeDataset(Writable*,
                      std::map< std::string, Argument > const&);
    void writeAttribute(Writable*,
                        std::map< std::string, Argument > const&);
    void readDataset(Writable*,
                     std::map< std::string, Argument > &);
    void readAttribute(Writable*,
                       std::map< std::string, Argument > &);
    void listPaths(Writable*,
                   std::map< std::string, Argument > &);
    void listDatasets(Writable*,
                      std::map< std::string, Argument > &);
    void listAttributes(Writable*,
                        std::map< std::string, Argument > &);
};
