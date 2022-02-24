/* Copyright 2018-2021 Franz Poeschel
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

#include <memory>
#include <string>

namespace openPMD
{
/**
 *  Wrapper around a shared pointer to:
 *  * a filename
 *  * and a boolean indicating whether the file still exists
 *  The wrapper adds no extra information, but some commodity functions.
 *  Invariant for any context within which this class shall be used:
 *  For any valid filename, there is at any time at most one
 *  such shared pointer (wrapper) known in said context's data structures
 *  (counting by pointer equality)
 *  This means, that a file can be invalidated (i.e. deleted or overwritten)
 *  by simply searching for one instance of the file among all known files and
 *  invalidating this instance
 *  A new instance may hence only be created after making sure that there are
 *  no valid instances in the data structures.
 */
struct InvalidatableFile
{
    explicit InvalidatableFile(std::string s);

    InvalidatableFile() = default;

    struct FileState
    {
        explicit FileState(std::string s);

        std::string name;
        bool valid = true;
    };

    std::shared_ptr<FileState> fileState;

    void invalidate();

    bool valid() const;

    InvalidatableFile &operator=(std::string s);

    bool operator==(InvalidatableFile const &f) const;

    std::string &operator*() const;

    std::string *operator->() const;

    explicit operator bool() const;
};
} // namespace openPMD

namespace std
{
template <>
struct hash<openPMD::InvalidatableFile>
{
    using argument_type = openPMD::InvalidatableFile;
    using result_type = std::size_t;

    result_type operator()(argument_type const &s) const noexcept;
};
} // namespace std
