/* Copyright 2021 Franz Poeschel
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

namespace openPMD
{
namespace json
{
    /**
     * @brief Merge two JSON/TOML datasets into one.
     *
     * Merging rules:
     * 1. If both `defaultValue` and `overwrite` are JSON/TOML objects, then the
     *    resulting JSON/TOML object will contain the union of both objects'
     *    keys. If a key is specified in both objects, the values corresponding
     *    to the key are merged recursively. Keys that point to a null value
     *    after this procedure will be pruned.
     * 2. In any other case, the JSON/TOML dataset `defaultValue` is replaced in
     *    its entirety with the JSON/TOML dataset `overwrite`.
     *
     * Note that item 2 means that datasets of different type will replace each
     * other without error.
     * It also means that array types will replace each other without any notion
     * of appending or merging.
     *
     * Possible use case:
     * An application uses openPMD-api and wants to do the following:
     * 1. Set some default backend options as JSON/TOML parameters.
     * 2. Let its users specify custom backend options additionally.
     *
     * By using the json::merge() function, this application can then allow
     * users to overwrite default options, while keeping any other ones.
     *
     * @param defaultValue A string containing either a JSON or a TOML dataset.
     * @param overwrite A string containing either a JSON or TOML dataset (does
     * not need to be the same as `defaultValue`).
     * @return std::string The merged dataset, according to the above rules. If
     * `defaultValue` was a JSON dataset, then as a JSON string, otherwise as a
     * TOML string.
     */
    std::string
    merge(std::string const &defaultValue, std::string const &overwrite);
} // namespace json
} // namespace openPMD
