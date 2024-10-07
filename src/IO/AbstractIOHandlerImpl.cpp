/* Copyright 2022 Franz Poeschel
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

#include "openPMD/IO/AbstractIOHandlerImpl.hpp"

#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/backend/Writable.hpp"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <type_traits>

namespace openPMD
{
AbstractIOHandlerImpl::AbstractIOHandlerImpl(AbstractIOHandler *handler)
    : m_handler{handler}
{
    if (auxiliary::getEnvNum("OPENPMD_VERBOSE", 0) != 0)
    {
        m_verboseIOTasks = true;
    }
}

namespace
{
    template <typename Vec>
    auto vec_as_string(Vec const &vec) -> std::string
    {
        if (vec.empty())
        {
            return "[]";
        }
        else
        {
            std::stringstream res;
            res << '[';
            auto it = vec.begin();
            res << *it++;
            auto end = vec.end();
            for (; it != end; ++it)
            {
                res << ", " << *it;
            }
            res << ']';
            return res.str();
        }
    }

    template <typename T, typename SFINAE = void>
    struct self_or_invoked
    {
        using type = T;
    };

    template <typename T>
    struct self_or_invoked<T, std::enable_if_t<std::is_invocable_v<T>>>
    {
        using type = std::invoke_result_t<T>;
    };

    template <typename T>
    using self_or_invoked_t = typename self_or_invoked<T>::type;

    template <typename DeferredString>
    auto undefer_string(DeferredString &&str)
        -> self_or_invoked_t<DeferredString &&>
    {
        if constexpr (std::is_invocable_v<DeferredString &&>)
        {
            return str();
        }
        else
        {
            return std::forward<DeferredString>(str);
        }
    }
} // namespace

template <typename... Args>
void AbstractIOHandlerImpl::writeToStderr([[maybe_unused]] Args &&...args) const
{
    if (m_verboseIOTasks)
    {
        (std::cerr << ... << undefer_string(args)) << std::endl;
    }
}

std::future<void> AbstractIOHandlerImpl::flush()
{
    using namespace auxiliary;

    while (!(*m_handler).m_work.empty())
    {
        IOTask &i = (*m_handler).m_work.front();
        try
        {
            switch (i.operation)
            {
                using O = Operation;
            case O::CREATE_FILE: {
                auto &parameter = deref_dynamic_cast<Parameter<O::CREATE_FILE>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] CREATE_FILE: ",
                    parameter.name);
                createFile(i.writable, parameter);
                break;
            }
            case O::CHECK_FILE: {
                auto &parameter = deref_dynamic_cast<Parameter<O::CHECK_FILE>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] CHECK_FILE: ",
                    parameter.name);
                checkFile(i.writable, parameter);
                break;
            }
            case O::CREATE_PATH: {
                auto &parameter = deref_dynamic_cast<Parameter<O::CREATE_PATH>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] CREATE_PATH: ",
                    parameter.path);
                createPath(i.writable, parameter);
                break;
            }
            case O::CREATE_DATASET: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::CREATE_DATASET>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] CREATE_DATASET: ",
                    parameter.name,
                    ", extent=",
                    [&parameter]() { return vec_as_string(parameter.extent); });
                createDataset(i.writable, parameter);
                break;
            }
            case O::EXTEND_DATASET: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::EXTEND_DATASET>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] EXTEND_DATASET");
                extendDataset(i.writable, parameter);
                break;
            }
            case O::OPEN_FILE: {
                auto &parameter = deref_dynamic_cast<Parameter<O::OPEN_FILE>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] OPEN_FILE: ",
                    parameter.name);
                openFile(i.writable, parameter);
                break;
            }
            case O::CLOSE_FILE: {
                auto &parameter = deref_dynamic_cast<Parameter<O::CLOSE_FILE>>(
                    i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] CLOSE_FILE");
                closeFile(i.writable, parameter);
                break;
            }
            case O::OPEN_PATH: {
                auto &parameter = deref_dynamic_cast<Parameter<O::OPEN_PATH>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] OPEN_PATH: ",
                    parameter.path);
                openPath(i.writable, parameter);
                break;
            }
            case O::CLOSE_PATH: {
                auto &parameter = deref_dynamic_cast<Parameter<O::CLOSE_PATH>>(
                    i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] CLOSE_PATH");
                closePath(i.writable, parameter);
                break;
            }
            case O::OPEN_DATASET: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::OPEN_DATASET>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] OPEN_DATASET: ",
                    parameter.name);
                openDataset(i.writable, parameter);
                break;
            }
            case O::DELETE_FILE: {
                auto &parameter = deref_dynamic_cast<Parameter<O::DELETE_FILE>>(
                    i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] DELETE_FILE");
                deleteFile(i.writable, parameter);
                break;
            }
            case O::DELETE_PATH: {
                auto &parameter = deref_dynamic_cast<Parameter<O::DELETE_PATH>>(
                    i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] DELETE_PATH");
                deletePath(i.writable, parameter);
                break;
            }
            case O::DELETE_DATASET: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::DELETE_DATASET>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] DELETE_DATASET");
                deleteDataset(i.writable, parameter);
                break;
            }
            case O::DELETE_ATT: {
                auto &parameter = deref_dynamic_cast<Parameter<O::DELETE_ATT>>(
                    i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] DELETE_ATT");
                deleteAttribute(i.writable, parameter);
                break;
            }
            case O::WRITE_DATASET: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::WRITE_DATASET>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] WRITE_DATASET");
                writeDataset(i.writable, parameter);
                break;
            }
            case O::WRITE_ATT: {
                auto &parameter = deref_dynamic_cast<Parameter<O::WRITE_ATT>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] WRITE_ATT: (",
                    parameter.dtype,
                    ") ",
                    parameter.name);
                writeAttribute(i.writable, parameter);
                break;
            }
            case O::READ_DATASET: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::READ_DATASET>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] READ_DATASET");
                readDataset(i.writable, parameter);
                break;
            }
            case O::GET_BUFFER_VIEW: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::GET_BUFFER_VIEW>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] GET_BUFFER_VIEW");
                getBufferView(i.writable, parameter);
                break;
            }
            case O::READ_ATT: {
                auto &parameter = deref_dynamic_cast<Parameter<O::READ_ATT>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] READ_ATT: ",
                    parameter.name);
                readAttribute(i.writable, parameter);
                break;
            }
            case O::LIST_PATHS: {
                auto &parameter = deref_dynamic_cast<Parameter<O::LIST_PATHS>>(
                    i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] LIST_PATHS");
                listPaths(i.writable, parameter);
                break;
            }
            case O::LIST_DATASETS: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::LIST_DATASETS>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] LIST_DATASETS");
                listDatasets(i.writable, parameter);
                break;
            }
            case O::LIST_ATTS: {
                auto &parameter = deref_dynamic_cast<Parameter<O::LIST_ATTS>>(
                    i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] LIST_ATTS");
                listAttributes(i.writable, parameter);
                break;
            }
            case O::ADVANCE: {
                auto &parameter = deref_dynamic_cast<Parameter<O::ADVANCE>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] ADVANCE ",
                    [&]() {
                        switch (parameter.mode)
                        {

                        case AdvanceMode::BEGINSTEP:
                            return "BEGINSTEP";
                        case AdvanceMode::ENDSTEP:
                            return "ENDSTEP";
                        }
                        throw std::runtime_error("Unreachable!");
                    }());
                advance(i.writable, parameter);
                break;
            }
            case O::AVAILABLE_CHUNKS: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::AVAILABLE_CHUNKS>>(
                        i.parameter.get());
                writeToStderr(
                    "[",
                    i.writable->parent,
                    "->",
                    i.writable,
                    "] AVAILABLE_CHUNKS");
                availableChunks(i.writable, parameter);
                break;
            }
            case O::DEREGISTER: {
                auto &parameter = deref_dynamic_cast<Parameter<O::DEREGISTER>>(
                    i.parameter.get());
                writeToStderr(
                    "[",
                    parameter.former_parent,
                    "->",
                    i.writable,
                    "] DEREGISTER");
                deregister(i.writable, parameter);
                break;
            }
            case O::TOUCH: {
                auto &parameter =
                    deref_dynamic_cast<Parameter<O::TOUCH>>(i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] TOUCH");
                touch(i.writable, parameter);
                break;
            }
            case O::SET_WRITTEN: {
                auto &parameter = deref_dynamic_cast<Parameter<O::SET_WRITTEN>>(
                    i.parameter.get());
                writeToStderr(
                    "[", i.writable->parent, "->", i.writable, "] SET_WRITTEN");
                setWritten(i.writable, parameter);
                break;
            }
            }
        }
        catch (...)
        {
            auto base_handler = [&i, this]() {
                std::cerr << "[AbstractIOHandlerImpl] IO Task "
                          << internal::operationAsString(i.operation)
                          << " failed with exception. Clearing IO queue and "
                             "passing on the exception."
                          << std::endl;
                while (!m_handler->m_work.empty())
                {
                    m_handler->m_work.pop();
                }
            };

            if (m_verboseIOTasks)
            {
                try
                {
                    // Throw again so we can distinguish between std::exception
                    // and other exception types.
                    throw;
                }
                catch (std::exception const &e)
                {
                    base_handler();
                    std::cerr << "Original exception: " << e.what()
                              << std::endl;
                    throw;
                }
                catch (...)
                {
                    base_handler();
                    throw;
                }
            }
            else
            {
                base_handler();
                throw;
            }
        }
        (*m_handler).m_work.pop();
    }
    return std::future<void>();
}

void AbstractIOHandlerImpl::setWritten(
    Writable *w, Parameter<Operation::SET_WRITTEN> const &param)
{
    w->written = param.target_status;
}
} // namespace openPMD
