/* Copyright 2017-2018 Fabian Koller
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

#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/Dataset.hpp"

#include <memory>
#include <map>
#include <vector>
#include <string>


namespace openPMD
{
class Attributable;
class Writable;

Writable*
getWritable(Attributable*);

/** Type of IO operation between logical and persistent data.
 */
enum class Operation
{
    CREATE_FILE,
    OPEN_FILE,
    DELETE_FILE,

    CREATE_PATH,
    OPEN_PATH,
    DELETE_PATH,
    LIST_PATHS,

    CREATE_DATASET,
    EXTEND_DATASET,
    OPEN_DATASET,
    DELETE_DATASET,
    WRITE_DATASET,
    READ_DATASET,
    LIST_DATASETS,

    DELETE_ATT,
    WRITE_ATT,
    READ_ATT,
    LIST_ATTS
};  //Operation

struct AbstractParameter
{
    virtual ~AbstractParameter() = default;
};

/** @brief Typesafe description of all required Arguments for a specified Operation.
 *
 * @note    Input operations (i.e. ones that transfer data from persistent files
 *          to logical representations in openPMD-api) use shared pointers to
 *          indicate shared ownership of the resource. The pointer will only be
 *          valid after the Operation has completed.
 * @tparam  Operation   Type of Operation to be executed.
 */
template< Operation >
struct Parameter : public AbstractParameter
{ };

template<>
struct Parameter< Operation::CREATE_FILE > : public AbstractParameter
{
    std::string name;
};

template<>
struct Parameter< Operation::OPEN_FILE > : public AbstractParameter
{
    std::string name;
};

template<>
struct Parameter< Operation::DELETE_FILE > : public AbstractParameter
{
    std::string name;
};

template<>
struct Parameter< Operation::CREATE_PATH > : public AbstractParameter
{
    std::string path;
};

template<>
struct Parameter< Operation::OPEN_PATH > : public AbstractParameter
{
    std::string path;
};

template<>
struct Parameter< Operation::DELETE_PATH > : public AbstractParameter
{
    std::string path;
};

template<>
struct Parameter< Operation::LIST_PATHS > : public AbstractParameter
{
    std::shared_ptr< std::vector< std::string > > paths
            = std::make_shared< std::vector< std::string > >();
};

template<>
struct Parameter< Operation::CREATE_DATASET > : public AbstractParameter
{
    std::string name;
    Extent extent;
    Datatype dtype;
    Extent chunkSize;
    std::string compression;
    std::string transform;
};

template<>
struct Parameter< Operation::EXTEND_DATASET > : public AbstractParameter
{
    std::string name;
    Extent extent;
};

template<>
struct Parameter< Operation::OPEN_DATASET > : public AbstractParameter
{
    std::string name;
    std::shared_ptr< Datatype > dtype
            = std::make_shared< Datatype >();
    std::shared_ptr< Extent > extent
            = std::make_shared< Extent >();
};

template<>
struct Parameter< Operation::DELETE_DATASET > : public AbstractParameter
{
    std::string name;
};

template<>
struct Parameter< Operation::WRITE_DATASET > : public AbstractParameter
{
    Extent extent;
    Offset offset;
    Datatype dtype;
    std::shared_ptr< void const > data;
};

template<>
struct Parameter< Operation::READ_DATASET > : public AbstractParameter
{
    Extent extent;
    Offset offset;
    Datatype dtype;
    std::shared_ptr< void > data;
};

template<>
struct Parameter< Operation::LIST_DATASETS > : public AbstractParameter
{
    std::shared_ptr< std::vector< std::string > > datasets
            = std::make_shared< std::vector< std::string > >();
};

template<>
struct Parameter< Operation::DELETE_ATT > : public AbstractParameter
{
    std::string name;
};

template<>
struct Parameter< Operation::WRITE_ATT > : public AbstractParameter
{
    Attribute::resource resource;
    std::string name;
    Datatype dtype;
};

template<>
struct Parameter< Operation::READ_ATT > : public AbstractParameter
{
    std::string name;
    std::shared_ptr< Datatype > dtype
            = std::make_shared< Datatype >();
    std::shared_ptr< Attribute::resource > resource
            = std::make_shared< Attribute::resource >();
};

template<>
struct Parameter< Operation::LIST_ATTS > : public AbstractParameter
{
    std::shared_ptr< std::vector< std::string > > attributes
            = std::make_shared< std::vector< std::string > >();
};


/** @brief Self-contained description of a single IO operation.
 *
 * Contained are
 * 1) the parameters to
 * 2) a single atomic file Operation on the
 * 3) concrete Writable object corresponding to both a local representation in
 *    openPMD-api and a persistent object in a file on disk
 */
class IOTask
{
public:
    /** Constructor for self-contained description of single IO operation.
     *
     * @tparam  op  Type of Operation to be executed.
     * @param   w   Writable indicating the location of the object being operated on.
     * @param   p   Parameter object supplying all required input and/or output parameters to the operation.
     */
    template< Operation op >
    IOTask(Writable* w,
           Parameter< op > const& p)
            : writable{w},
              operation{op},
              parameter{new Parameter< op >(p)}
    { }

    template< Operation op >
    IOTask(Attributable* a,
           Parameter< op > const& p)
            : writable{getWritable(a)},
              operation{op},
              parameter{new Parameter< op >(p)}
    { }

    Writable* writable;
    Operation operation;
    std::shared_ptr< AbstractParameter > parameter;
};  //IOTask
} // openPMD
