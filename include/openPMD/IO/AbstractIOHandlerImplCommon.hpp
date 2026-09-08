/* Copyright 2018-2025 Franz Poeschel, Axel Huebl
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

#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace openPMD
{
template <
    typename IOHandlerImpl_t,
    typename FilePositionType = typename IOHandlerImpl_t::FilePositionType>
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
    // MUST be an ordered set in order to consistently flush on different
    // parallel processes (same logic cant apply to m_files since Writable*
    // pointers are not predictable)
    std::set<internal::SharedFileState> m_dirty;
    std::unordered_map<std::string, internal::SharedFileState> m_files;

    internal::SharedFileState &
    makeFile(Writable *, std::string fileName, bool consider_open_files);

    void associateWithFile(Writable *writable, internal::SharedFileState file);

    /**
     *
     * @return Full OS path of the file.
     */
    std::string fullPath(internal::FileState const &);

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
    internal::FileState &
    refreshFileFromParent(Writable *writable, bool preferParentFile);

    /**
     * Figure out the file position of the writable.
     * Only modify the writable's fileposition when specified.
     * @param writable The writable.
     * @return The current file position.
     */
    FilePositionType *setAndGetFilePosition(Writable *writable);

    /**
     * Figure out the file position of the writable and extend it.
     * @param writable The writable.
     * @param extend The extension string.
     * @return The current file position.
     */
    FilePositionType *
    setAndGetFilePosition(Writable *writable, std::string extend);

    /*
     * The "virtual" methods here must be implemented by the child class,
     * but the references are resolved at compile time, hence not really
     * virtual.
     */
#if 0
    /**
     * @return A string representation of the file position.
     */
    virtual std::string filePositionToString(FilePositionType const &) = 0;

    /**
     * @return A new file position that is extended with the given string.
     */
    virtual std::shared_ptr< IOHandler_t,  FilePositionType>
    extendFilePosition(FilePositionType const &, std::string) = 0;
#endif
};

template <typename IOHandler_t, typename FilePositionType>
AbstractIOHandlerImplCommon<IOHandler_t, FilePositionType>::
    AbstractIOHandlerImplCommon(AbstractIOHandler *handler)
    : AbstractIOHandlerImpl{handler}
{}

template <typename IOHandler_t, typename FilePositionType>
AbstractIOHandlerImplCommon<IOHandler_t, FilePositionType>::
    ~AbstractIOHandlerImplCommon() = default;

template <typename IOHandler_t, typename FilePositionType>
internal::SharedFileState &
AbstractIOHandlerImplCommon<IOHandler_t, FilePositionType>::makeFile(
    Writable *writable, std::string file, bool consider_open_files)
{
    auto make_new = [&]() {
        writable->fileState =
            std::make_shared<std::optional<internal::FileState>>(file);
        m_files[std::move(file)] = writable->fileState;
    };
    if (consider_open_files)
    {
        if (auto it = m_files.find(file);
            it != m_files.end() && it->second->has_value())
        {
            writable->fileState = it->second;
        }
        else
        {
            make_new();
        }
    }

    else
    {
        make_new();
    }

    return writable->fileState;
}

template <typename IOHandler_t, typename FilePositionType>
void AbstractIOHandlerImplCommon<IOHandler_t, FilePositionType>::
    associateWithFile(Writable *writable, internal::SharedFileState file)
{
    writable->fileState = std::move(file);
}

template <typename IOHandler_t, typename FilePositionType>
std::string
AbstractIOHandlerImplCommon<IOHandler_t, FilePositionType>::fullPath(
    internal::FileState const &file)
{
    return fullPath(file.name);
}

template <typename IOHandler_t, typename FilePositionType>
std::string
AbstractIOHandlerImplCommon<IOHandler_t, FilePositionType>::fullPath(
    std::string fileName)
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

template <typename IOHandler_t, typename FilePositionType>
internal::FileState &
AbstractIOHandlerImplCommon<IOHandler_t, FilePositionType>::
    refreshFileFromParent(Writable *writable, bool preferParentFile)
{
    auto getFileFromParent = [writable, this]() -> internal::FileState & {
        auto search = writable->parent;
        if (!search)
        {
            throw std::runtime_error(
                "[refreshFileFromParent] No parent found.");
        }
        while (!search->fileState->has_value())
        {
            if (!search->parent)
            {
                throw std::runtime_error(
                    "[refreshFileFromParent] No active file in the ancestors.");
            }
            search = search->parent;
        }
        auto &file = search->fileState;
        search = writable->parent;
        while (!search->fileState->has_value())
        {
            search->fileState = file;
            search = search->parent;
        }
        associateWithFile(writable, file);
        return **file;
    };
    if (preferParentFile && writable->parent)
    {
        return getFileFromParent();
    }
    else
    {
        if (writable->fileState && writable->fileState->has_value())
        {
            return **writable->fileState;
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

template <typename IOHandlerImpl_t, typename FilePositionType>
auto AbstractIOHandlerImplCommon<IOHandlerImpl_t, FilePositionType>::
    setAndGetFilePosition(Writable *writable) -> FilePositionType *
{
    std::shared_ptr<AbstractFilePosition> new_pos;

    if (writable->abstractFilePosition)
    {
        new_pos = writable->abstractFilePosition;
    }
    else if (writable->parent)
    {
        new_pos = writable->parent->abstractFilePosition;
    }
    else
    { // we are root
        new_pos = std::make_shared<FilePositionType>();
    }
    auto res = new_pos.get();
    writable->abstractFilePosition = std::move(new_pos);
    return dynamic_cast<FilePositionType *>(res);
}

template <typename IOHandlerImpl_t, typename FilePositionType>
auto AbstractIOHandlerImplCommon<IOHandlerImpl_t, FilePositionType>::
    setAndGetFilePosition(Writable *writable, std::string extend)
        -> FilePositionType *
{
    if (extend.empty())
    {
        return setAndGetFilePosition(writable);
    }
    if (writable->abstractFilePosition)
    {
        return dynamic_cast<FilePositionType *>(
            writable->abstractFilePosition.get());
    }
    if (!auxiliary::starts_with(extend, '/') &&
        auxiliary::ends_with(extend, '/'))
    {
        extend = "/" + extend.substr(0, extend.size() - 1);
    }
    else if (!auxiliary::starts_with(extend, '/'))
    {
        extend = "/" + extend;
    }
    else if (auxiliary::ends_with(extend, '/'))
    {
        extend = extend.substr(0, extend.size() - 1);
    }
    auto oldPos = setAndGetFilePosition(writable);
    auto new_pos = static_cast<IOHandlerImpl_t *>(this)->extendFilePosition(
        *oldPos, extend);

    auto res = new_pos.get();
    writable->abstractFilePosition = std::move(new_pos);
    return dynamic_cast<FilePositionType *>(res);
}
} // namespace openPMD
