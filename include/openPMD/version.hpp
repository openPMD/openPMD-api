/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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

#include <map>
#include <string>
#include <vector>

/** version of the openPMD-api library (compile-time)
 * @{
 */
#define OPENPMDAPI_VERSION_MAJOR 0
#define OPENPMDAPI_VERSION_MINOR 14
#define OPENPMDAPI_VERSION_PATCH 5
#define OPENPMDAPI_VERSION_LABEL ""
/** @} */

/** maximum supported version of the openPMD standard (read & write,
 * compile-time)
 * @{
 */
#define OPENPMD_STANDARD_MAJOR 1
#define OPENPMD_STANDARD_MINOR 1
#define OPENPMD_STANDARD_PATCH 0
/** @} */

/** minimum supported version of the openPMD standard (read, compile-time)
 * @{
 */
#define OPENPMD_STANDARD_MIN_MAJOR 1
#define OPENPMD_STANDARD_MIN_MINOR 0
#define OPENPMD_STANDARD_MIN_PATCH 0
/** @} */

/** convert major, minor, patch version into a 1000th-interleaved number
 */
#define OPENPMDAPI_VERSIONIFY(major, minor, patch)                             \
    (major * 1000000 + minor * 1000 + patch)

/** Compare if the library version is greater or equal than major,minor,patch
 */
#define OPENPMDAPI_VERSION_GE(major, minor, patch)                             \
    (OPENPMDAPI_VERSIONIFY(                                                    \
         OPENPMDAPI_VERSION_MAJOR,                                             \
         OPENPMDAPI_VERSION_MINOR,                                             \
         OPENPMDAPI_VERSION_PATCH) >=                                          \
     OPENPMDAPI_VERSIONIFY(major, minor, patch))

namespace openPMD
{
/** Return the version of the openPMD-api library (run-time)
 *
 * @return std::string API version (dot separated)
 */
std::string getVersion();

/** Return the maximum supported version of the openPMD standard (read & write,
 * run-time)
 *
 * @return std::string openPMD standard version (dot separated)
 */
std::string getStandard();

/** Return the minimum supported version of the openPMD standard (read,
 * run-time)
 *
 * @return std::string minimum openPMD standard version (dot separated)
 */
std::string getStandardMinimum();

/** Return the feature variants of the openPMD-api library (run-time)
 *
 * @return std::map< std::string, bool > with variants such as backends
 */
std::map<std::string, bool> getVariants();

/** Return the file extensions supported in this variant of the openPMD-api
 * library (run-time)
 *
 * @return std::vector< std::string > with file extensions
 */
std::vector<std::string> getFileExtensions();

} // namespace openPMD
