/* Copyright 2018-2019 Fabian Koller
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
#include "openPMD/auxiliary/StringManip.hpp"

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
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path.c_str());

    return (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat s;
    return (0 == stat(path.c_str(), &s)) && S_ISDIR(s.st_mode);
#endif
}

bool
file_exists( std::string const& path )
{
#ifdef _WIN32
    DWORD attributes = GetFileAttributes(path.c_str());

    return (attributes != INVALID_FILE_ATTRIBUTES &&
            !(attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
    struct stat s;
    return (0 == stat(path.c_str(), &s)) && S_ISREG(s.st_mode);
#endif
}

std::vector< std::string >
list_directory(std::string const& path )
{
    std::vector< std::string > ret;
#ifdef _WIN32
    std::string pattern(path);
    pattern.append("\\*");
    WIN32_FIND_DATA data;
    HANDLE hFind = FindFirstFile(pattern.c_str(), &data);
    if( hFind == INVALID_HANDLE_VALUE )
        throw std::system_error(std::error_code(errno, std::system_category()));
    do {
        if( strcmp(data.cFileName, ".") != 0 && strcmp(data.cFileName, "..") != 0 )
            ret.emplace_back(data.cFileName);
    } while (FindNextFile(hFind, &data) != 0);
    FindClose(hFind);
#else
    auto directory = opendir(path.c_str());
    if( !directory )
        throw std::system_error(std::error_code(errno, std::system_category()));
    dirent* entry;
    while ((entry = readdir(directory)) != nullptr)
        if( strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 )
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

#ifdef _WIN32
    auto mk = [](std::string const& p) -> bool { return CreateDirectory(p.c_str(), nullptr); };
#else
    mode_t mask = umask(0);
    umask(mask);
    auto mk = [mask](std::string const& p) -> bool { return (0 == mkdir(p.c_str(), 0777 & ~mask));};
#endif
    std::istringstream ss(path);
    std::string token;

    std::string partialPath;
    if( auxiliary::starts_with(path, directory_separator) )
        partialPath += directory_separator;
    bool success = true;
    while( std::getline( ss, token, directory_separator ) )
    {
        if( !token.empty() )
            partialPath += token + directory_separator;
        if( !directory_exists( partialPath ) )
        {
            bool partial_success = mk(partialPath);
            if( !partial_success )
                // did someone else just race us to create this dir?
                if( !directory_exists( partialPath ) )
                    success = success && partial_success;
        }
    }
    return success;
}

bool
remove_directory( std::string const& path )
{
    if( !directory_exists(path) )
        return false;

    bool success = true;
#ifdef _WIN32
    auto del = [](std::string const& p) -> bool { return RemoveDirectory(p.c_str()); };
#else
    auto del = [](std::string const& p) -> bool { return (0 == remove(p.c_str()));};
#endif
    for( auto const& entry : list_directory(path) )
    {
        std::string partialPath = path + directory_separator + entry;
        if( directory_exists(partialPath) )
            success &= remove_directory(partialPath);
        else if( file_exists(partialPath) )
            success &= remove_file(partialPath);
    }
    success &= del(path);
    return success;
}

bool
remove_file( std::string const& path )
{
  if( !file_exists(path) )
      return false;

#ifdef _WIN32
    return DeleteFile(path.c_str());
#else
    return (0 == remove(path.c_str()));
#endif
}  

} // auxiliary
} // openPMD
