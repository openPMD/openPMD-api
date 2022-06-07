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
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/backend/Attributable.hpp"

namespace openPMD
{
Writable *getWritable(AttributableInterface *a)
{
    return &a->writable();
}

namespace internal
{
    std::string operationAsString(Operation op)
    {
        switch (op)
        {
        case Operation::CREATE_FILE:
            return "CREATE_FILE";
            break;
        case Operation::OPEN_FILE:
            return "OPEN_FILE";
            break;
        case Operation::CLOSE_FILE:
            return "CLOSE_FILE";
            break;
        case Operation::DELETE_FILE:
            return "DELETE_FILE";
            break;
        case Operation::CREATE_PATH:
            return "CREATE_PATH";
            break;
        case Operation::CLOSE_PATH:
            return "CLOSE_PATH";
            break;
        case Operation::OPEN_PATH:
            return "OPEN_PATH";
            break;
        case Operation::DELETE_PATH:
            return "DELETE_PATH";
            break;
        case Operation::LIST_PATHS:
            return "LIST_PATHS";
            break;
        case Operation::CREATE_DATASET:
            return "CREATE_DATASET";
            break;
        case Operation::EXTEND_DATASET:
            return "EXTEND_DATASET";
            break;
        case Operation::OPEN_DATASET:
            return "OPEN_DATASET";
            break;
        case Operation::DELETE_DATASET:
            return "DELETE_DATASET";
            break;
        case Operation::WRITE_DATASET:
            return "WRITE_DATASET";
            break;
        case Operation::READ_DATASET:
            return "READ_DATASET";
            break;
        case Operation::LIST_DATASETS:
            return "LIST_DATASETS";
            break;
        case Operation::GET_BUFFER_VIEW:
            return "GET_BUFFER_VIEW";
            break;
        case Operation::DELETE_ATT:
            return "DELETE_ATT";
            break;
        case Operation::WRITE_ATT:
            return "WRITE_ATT";
            break;
        case Operation::READ_ATT:
            return "READ_ATT";
            break;
        case Operation::LIST_ATTS:
            return "LIST_ATTS";
            break;
        case Operation::ADVANCE:
            return "ADVANCE";
            break;
        case Operation::AVAILABLE_CHUNKS:
            return "AVAILABLE_CHUNKS";
            break;
        default:
            return "unknown";
            break;
        }
    }
} // namespace internal
} // namespace openPMD
