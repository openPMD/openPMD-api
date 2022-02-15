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
#pragma once

#include <string>
#include <vector>

#include "openPMD/config.hpp"

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

namespace openPMD
{
namespace auxiliary
{
#ifdef _WIN32
    static constexpr char const directory_separator = '\\';
#else
    static constexpr char const directory_separator = '/';
#endif

    /** Check if a directory exists at a give absolute or relative path.
     *
     * @param   path    Absolute or relative path to examine.
     * @return  true if the given path or file status corresponds to an existing
     * directory, false otherwise.
     */
    bool directory_exists(std::string const &path);

    /** Check if a file exists at a given absolute or relative path.
     *
     * @param   path    Absolute or relative path to examine.
     * @return  true if the given path or file status corresponds to an existing
     * file, false otherwise.
     */
    bool file_exists(std::string const &path);

    /** List all contents of a directory at a given absolute or relative path.
     *
     * @note    The equivalent of `ls path`
     * @note    Both contained files and directories are listed.
     *          `.` and `..` are not returned.
     * @throw   std::system_error when the given path is not a valid directory.
     * @param   path    Absolute or relative path of directory to examine.
     * @return  Vector of all contained files and directories.
     */
    std::vector<std::string> list_directory(std::string const &path);

    /** Create all required directories to have a reachable given absolute or
     * relative path.
     *
     * @note    The equivalent of `mkdir -p path`
     * @param   path    Absolute or relative path to the new directory to
     * create.
     * @return  true if a directory was created for the directory p resolves to,
     * false otherwise.
     */
    bool create_directories(std::string const &path);

    /** Remove the directory identified by the given path.
     *
     * @note    The equivalent of `rm -r path`.
     * @param   path    Absolute or relative path to the directory to delete.
     * @return  true if the directory was deleted, false otherwise and if it did
     * not exist.
     */
    bool remove_directory(std::string const &path);

    /** Remove the file identified by the given path.
     *
     * @note    The equivalent of `rm path`.
     * @param   path    Absolute or relative path to the file to delete.
     * @return  true if the file was deleted, false otherwise and if it did not
     * exist.
     */
    bool remove_file(std::string const &path);

#if openPMD_HAVE_MPI

    std::string collective_file_read(std::string const &path, MPI_Comm);

#endif
} // namespace auxiliary
} // namespace openPMD
