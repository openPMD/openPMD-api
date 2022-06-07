/* Copyright 2018-2021 Fabian Koller
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
#include "openPMD/auxiliary/Unused.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <system_error>

namespace openPMD
{
namespace auxiliary
{
    bool directory_exists(std::string const &path)
    {
#ifdef _WIN32
        DWORD attributes = GetFileAttributes(path.c_str());

        return (
            attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat s;
        return (0 == stat(path.c_str(), &s)) && S_ISDIR(s.st_mode);
#endif
    }

    bool file_exists(std::string const &path)
    {
#ifdef _WIN32
        DWORD attributes = GetFileAttributes(path.c_str());

        return (
            attributes != INVALID_FILE_ATTRIBUTES &&
            !(attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
        struct stat s;
        return (0 == stat(path.c_str(), &s)) && S_ISREG(s.st_mode);
#endif
    }

    std::vector<std::string> list_directory(std::string const &path)
    {
        std::vector<std::string> ret;
#ifdef _WIN32
        std::string pattern(path);
        pattern.append("\\*");
        WIN32_FIND_DATA data;
        HANDLE hFind = FindFirstFile(pattern.c_str(), &data);
        if (hFind == INVALID_HANDLE_VALUE)
            throw std::system_error(
                std::error_code(errno, std::system_category()));
        do
        {
            if (strcmp(data.cFileName, ".") != 0 &&
                strcmp(data.cFileName, "..") != 0)
                ret.emplace_back(data.cFileName);
        } while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
#else
        auto directory = opendir(path.c_str());
        if (!directory)
            throw std::system_error(
                std::error_code(errno, std::system_category()));
        dirent *entry;
        while ((entry = readdir(directory)) != nullptr)
            if (strcmp(entry->d_name, ".") != 0 &&
                strcmp(entry->d_name, "..") != 0)
                ret.emplace_back(entry->d_name);
        closedir(directory);
#endif
        return ret;
    }

    bool create_directories(std::string const &path)
    {
        if (directory_exists(path))
            return true;

#ifdef _WIN32
        auto mk = [](std::string const &p) -> bool {
            return CreateDirectory(p.c_str(), nullptr);
        };
#else
        mode_t mask = umask(0);
        umask(mask);
        auto mk = [mask](std::string const &p) -> bool {
            return (0 == mkdir(p.c_str(), 0777 & ~mask));
        };
#endif
        std::istringstream ss(path);
        std::string token;

        std::string partialPath;
        if (auxiliary::starts_with(path, directory_separator))
            partialPath += directory_separator;
        bool success = true;
        while (std::getline(ss, token, directory_separator))
        {
            if (!token.empty())
                partialPath += token + directory_separator;
            if (!directory_exists(partialPath))
            {
                bool partial_success = mk(partialPath);
                if (!partial_success)
                    // did someone else just race us to create this dir?
                    if (!directory_exists(partialPath))
                        success = success && partial_success;
            }
        }
        return success;
    }

    bool remove_directory(std::string const &path)
    {
        if (!directory_exists(path))
            return false;

        bool success = true;
#ifdef _WIN32
        auto del = [](std::string const &p) -> bool {
            return RemoveDirectory(p.c_str());
        };
#else
        auto del = [](std::string const &p) -> bool {
            return (0 == remove(p.c_str()));
        };
#endif
        for (auto const &entry : list_directory(path))
        {
            std::string partialPath = path + directory_separator + entry;
            if (directory_exists(partialPath))
                success &= remove_directory(partialPath);
            else if (file_exists(partialPath))
                success &= remove_file(partialPath);
        }
        success &= del(path);
        return success;
    }

    bool remove_file(std::string const &path)
    {
        if (!file_exists(path))
            return false;

#ifdef _WIN32
        return DeleteFile(path.c_str());
#else
        return (0 == remove(path.c_str()));
#endif
    }

#if openPMD_HAVE_MPI

    namespace
    {
        template <typename>
        struct MPI_Types;

        template <>
        struct MPI_Types<unsigned long>
        {
            static MPI_Datatype const value;
        };

        template <>
        struct MPI_Types<unsigned long long>
        {
            static MPI_Datatype const value;
        };

        template <>
        struct MPI_Types<unsigned>
        {
            static MPI_Datatype const value;
        };

        /*
         * Only some of these are actually instanciated,
         * so suppress warnings for the others.
         */
        OPENPMDAPI_UNUSED
        MPI_Datatype const MPI_Types<unsigned>::value = MPI_UNSIGNED;
        OPENPMDAPI_UNUSED
        MPI_Datatype const MPI_Types<unsigned long>::value = MPI_UNSIGNED_LONG;
        OPENPMDAPI_UNUSED
        MPI_Datatype const MPI_Types<unsigned long long>::value =
            MPI_UNSIGNED_LONG_LONG;
    } // namespace

    std::string collective_file_read(std::string const &path, MPI_Comm comm)
    {
        int rank, size;
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);

        std::string res;
        size_t stringLength = 0;
        if (rank == 0)
        {
            std::fstream handle;
            handle.open(path, std::ios_base::in);
            std::stringstream stream;
            stream << handle.rdbuf();
            res = stream.str();
            if (!handle.good())
            {
                throw std::runtime_error(
                    "Failed reading JSON config from file " + path + ".");
            }
            stringLength = res.size() + 1;
        }
        MPI_Datatype datatype = MPI_Types<size_t>::value;
        int err = MPI_Bcast(&stringLength, 1, datatype, 0, comm);
        if (err)
        {
            throw std::runtime_error(
                "[collective_file_read] MPI_Bcast stringLength failure.");
        }
        std::vector<char> recvbuf(stringLength, 0);
        if (rank == 0)
        {
            std::copy_n(res.c_str(), stringLength, recvbuf.data());
        }
        err = MPI_Bcast(recvbuf.data(), stringLength, MPI_CHAR, 0, comm);
        if (err)
        {
            throw std::runtime_error(
                "[collective_file_read] MPI_Bcast file content failure.");
        }
        if (rank != 0)
        {
            res = recvbuf.data();
        }
        return res;
    }

#endif

} // namespace auxiliary
} // namespace openPMD
