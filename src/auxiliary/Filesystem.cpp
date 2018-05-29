/* Copyright 2018 Fabian Koller
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
#include "openPMD/auxiliary/Filesystem.hpp"

#ifdef _WIN32
#   include <windows.h>
#else
#   include <cstring>
#   include <sys/stat.h>
#   include <sys/types.h>
#   include <dirent.h>
#endif

#include <stdexcept>
#include <system_error>


namespace openPMD
{
namespace auxiliary
{
bool
directory_exists(std::string const& path)
{
    bool exists = false;
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path.c_str());

    exists = (attributes != INVALID_FILE_ATTRIBUTES &&
             (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat s;
    bool success = (stat(path.c_str(), &s) == 0);
    exists = success && S_ISDIR(s.st_mode);
#endif
    return exists;
}

bool
file_exists( std::string const& path )
{
    bool exists = false;
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path.c_str());

    exists = (attributes != INVALID_FILE_ATTRIBUTES &&
             !(attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat s;
    bool success = (stat(path.c_str(), &s) == 0);
    exists = success && S_ISREG(s.st_mode);
#endif
    return exists;
}

std::vector< std::string >
list_directory(std::string const& path )
{
  std::vector< std::string > ret;
#ifdef _WIN32
    std::string pattern(path);
    pattern.append("\\*");
    WIN32_FIND_DATA data;
    HANDLE hFind;
    if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) {
        do {
            ret.push_back(data.cFileName);
        } while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
    }
#else
    auto directory = opendir(path.c_str());
    if( !directory )
        throw std::system_error(std::error_code(errno, std::system_category()));
    dirent* entry;
    while ((entry = readdir(directory)) != nullptr)
        if( strncmp(entry->d_name, ".", 1) && strncmp(entry->d_name, "..", 2) )
            ret.emplace_back(entry->d_name);
    closedir(directory);
#endif
    return ret;
}

bool
create_directories( std::string const& path )
{
    if( directory_exists(path) )
        return true;

    bool success = false;
#ifdef _WIN32
    success = CreateDirectory(path.c_str(), NULL);
#else
    success = (mkdir(path.c_str(), 0777) == 0);
#endif
    return success;
}

bool
remove_directory( std::string const& path )
{
    if( !directory_exists(path) )
        return false;

    bool success = false;
#ifdef _WIN32
    success = RemoveDirectory(path.c_str());
#else
    success = (remove(path.c_str()) == 0);
#endif
    return success;
}

bool
remove_file( std::string const& path )
{
  if( !file_exists(path) )
      return false;

    bool success = false;
#ifdef _WIN32
    success = DeleteFile(path.c_str());
#else
    success = (remove(path.c_str()) == 0);
#endif
    return success;
}
} // auxiliary
} // openPMD
