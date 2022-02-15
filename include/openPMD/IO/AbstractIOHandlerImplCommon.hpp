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

#include "openPMD/IO/AbstractFilePosition.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/IO/InvalidatableFile.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/backend/Writable.hpp"

#include <unordered_map>
#include <unordered_set>

namespace openPMD
{
template <typename FilePositionType = AbstractFilePosition>
class AbstractIOHandlerImplCommon : public AbstractIOHandlerImpl
{
    // friend struct detail::BufferedActions;
public:
    explicit AbstractIOHandlerImplCommon(AbstractIOHandler *handler);

    ~AbstractIOHandlerImplCommon() override;

protected:
    /**
     * map each Writable to its associated file contains only the filename,
     * without the OS path
     */
    std::unordered_map<Writable *, InvalidatableFile> m_files;
    std::unordered_set<InvalidatableFile> m_dirty;

    enum PossiblyExisting
    {
        PE_InvalidatableFile = 0,
        PE_Iterator,
        PE_NewlyCreated,
    };

    std::tuple<
        InvalidatableFile,
        std::unordered_map<Writable *, InvalidatableFile>::iterator,
        bool>
    getPossiblyExisting(std::string file);

    void associateWithFile(Writable *writable, InvalidatableFile file);

    /**
     *
     * @return Full OS path of the file.
     */
    std::string fullPath(InvalidatableFile);

    std::string fullPath(std::string);

    /**
     * Get the writable's containing file.
     * @param writable The writable whose containing file to figure out.
     * @param preferParentFile If true, the file is set to the parent's file if
     *     present. Otherwise, the parent file is only considered if no own file
     *     is defined. This is usually needed when switching between iterations
     * when opening paths.
     * @return The containing file of the writable. If its parent is associated
     * with another file, update the writable to match its parent and return
     * the refreshed file.
     */
    InvalidatableFile
    refreshFileFromParent(Writable *writable, bool preferParentFile);

    /**
     * Figure out the file position of the writable.
     * Only modify the writable's fileposition when specified.
     * @param writable The writable.
     * @param write Whether to refresh the writable's file position.
     * @return The current file position.
     */
    std::shared_ptr<FilePositionType>
    setAndGetFilePosition(Writable *writable, bool write = true);

    /**
     * Figure out the file position of the writable and extend it.
     * @param writable The writable.
     * @param extend The extension string.
     * @return The current file position.
     */
    virtual std::shared_ptr<FilePositionType>
    setAndGetFilePosition(Writable *writable, std::string extend);

    /**
     * @return A string representation of the file position.
     */
    virtual std::string
        filePositionToString(std::shared_ptr<FilePositionType>) = 0;

    /**
     * @return A new file position that is extended with the given string.
     */
    virtual std::shared_ptr<FilePositionType> extendFilePosition(
        std::shared_ptr<FilePositionType> const &, std::string) = 0;
};

template <typename FilePositionType>
AbstractIOHandlerImplCommon<FilePositionType>::AbstractIOHandlerImplCommon(
    AbstractIOHandler *handler)
    : AbstractIOHandlerImpl{handler}
{}

template <typename FilePositionType>
AbstractIOHandlerImplCommon<FilePositionType>::~AbstractIOHandlerImplCommon() =
    default;

template <typename FilePositionType>
std::tuple<
    InvalidatableFile,
    std::unordered_map<Writable *, InvalidatableFile>::iterator,
    bool>
AbstractIOHandlerImplCommon<FilePositionType>::getPossiblyExisting(
    std::string file)
{

    auto it = std::find_if(
        m_files.begin(),
        m_files.end(),
        [file](
            std::unordered_map<Writable *, InvalidatableFile>::value_type const
                &entry) {
            return *entry.second == file && entry.second.valid();
        });

    bool newlyCreated;
    InvalidatableFile name;
    if (it == m_files.end())
    {
        name = file;
        newlyCreated = true;
    }
    else
    {
        name = it->second;
        newlyCreated = false;
    }
    return std::tuple<
        InvalidatableFile,
        std::unordered_map<Writable *, InvalidatableFile>::iterator,
        bool>(std::move(name), it, newlyCreated);
}

template <typename FilePositionType>
void AbstractIOHandlerImplCommon<FilePositionType>::associateWithFile(
    Writable *writable, InvalidatableFile file)
{
    // make sure to overwrite
    m_files[writable] = std::move(file);
}

template <typename FilePositionType>
std::string AbstractIOHandlerImplCommon<FilePositionType>::fullPath(
    InvalidatableFile fileName)
{
    return fullPath(*fileName);
}

template <typename FilePositionType>
std::string
AbstractIOHandlerImplCommon<FilePositionType>::fullPath(std::string fileName)
{
    if (auxiliary::ends_with(m_handler->directory, "/"))
    {
        return m_handler->directory + fileName;
    }
    else
    {
        return m_handler->directory + "/" + fileName;
    }
}

template <typename FilePositionType>
InvalidatableFile
AbstractIOHandlerImplCommon<FilePositionType>::refreshFileFromParent(
    Writable *writable, bool preferParentFile)
{
    auto getFileFromParent = [writable, this]() {
        auto file = m_files.find(writable->parent)->second;
        associateWithFile(writable, file);
        return file;
    };
    if (preferParentFile && writable->parent)
    {
        return getFileFromParent();
    }
    else
    {
        auto it = m_files.find(writable);
        if (it != m_files.end())
        {
            return m_files.find(writable)->second;
        }
        else if (writable->parent)
        {
            return getFileFromParent();
        }
        else
        {
            throw std::runtime_error(
                "Internal error: Root object must be opened explicitly.");
        }
    }
}

template <typename FilePositionType>
std::shared_ptr<FilePositionType>
AbstractIOHandlerImplCommon<FilePositionType>::setAndGetFilePosition(
    Writable *writable, bool write)
{
    std::shared_ptr<AbstractFilePosition> res;

    if (writable->abstractFilePosition)
    {
        res = writable->abstractFilePosition;
    }
    else if (writable->parent)
    {
        res = writable->parent->abstractFilePosition;
    }
    else
    { // we are root
        res = std::make_shared<FilePositionType>();
    }
    if (write)
    {
        writable->abstractFilePosition = res;
    }
    return std::dynamic_pointer_cast<FilePositionType>(res);
}

template <typename FilePositionType>
std::shared_ptr<FilePositionType>
AbstractIOHandlerImplCommon<FilePositionType>::setAndGetFilePosition(
    Writable *writable, std::string extend)
{
    if (!auxiliary::starts_with(extend, '/'))
    {
        extend = "/" + extend;
    }
    auto oldPos = setAndGetFilePosition(writable, false);
    auto res = extendFilePosition(oldPos, extend);

    writable->abstractFilePosition = res;
    return res;
}
} // namespace openPMD
