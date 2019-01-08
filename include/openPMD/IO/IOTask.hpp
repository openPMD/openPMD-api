/* Copyright 2017-2019 Fabian Koller
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
#include <utility>


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
    AbstractParameter() = default;
    //AbstractParameter(AbstractParameter&&) = default;

    // avoid object slicing
    AbstractParameter(const AbstractParameter&) = delete;
    AbstractParameter& operator=(const AbstractParameter&) = delete;
    virtual std::unique_ptr< AbstractParameter > clone() const = 0;
};

/** @brief Typesafe description of all required arguments for a specified Operation.
 *
 * @note    Input operations (i.e. ones that transfer data from persistent files
 *          to logical representations in openPMD-api) use shared pointers to
 *          indicate shared ownership of the resource. The pointer will only be
 *          valid after the Operation has completed.
 * @tparam  Operation   Type of Operation to be executed.
 */
template< Operation >
struct Parameter : public AbstractParameter
{
    Parameter() = delete;
    Parameter(Parameter const &) = delete;
    Parameter(Parameter &&) = delete;
};

template<>
struct Parameter< Operation::CREATE_FILE > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), name(p.name) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::CREATE_FILE >(*this));
    }

    std::string name = "";
};

template<>
struct Parameter< Operation::OPEN_FILE > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), name(p.name) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::OPEN_FILE >(*this));
    }

    std::string name = "";
};

template<>
struct Parameter< Operation::DELETE_FILE > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), name(p.name) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::DELETE_FILE >(*this));
    }

    std::string name = "";
};

template<>
struct Parameter< Operation::CREATE_PATH > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), path(p.path) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::CREATE_PATH >(*this));
    }

    std::string path = "";
};

template<>
struct Parameter< Operation::OPEN_PATH > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), path(p.path) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::OPEN_PATH >(*this));
    }

    std::string path = "";
};

template<>
struct Parameter< Operation::DELETE_PATH > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), path(p.path) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::DELETE_PATH >(*this));
    }

    std::string path = "";
};

template<>
struct Parameter< Operation::LIST_PATHS > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), paths(p.paths) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::LIST_PATHS >(*this));
    }

    std::shared_ptr< std::vector< std::string > > paths
            = std::make_shared< std::vector< std::string > >();
};

template<>
struct Parameter< Operation::CREATE_DATASET > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        name(p.name), extent(p.extent), dtype(p.dtype),
        chunkSize(p.chunkSize), compression(p.compression),
        transform(p.transform) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::CREATE_DATASET >(*this));
    }

    std::string name = "";
    Extent extent = {};
    Datatype dtype = Datatype::UNDEFINED;
    Extent chunkSize = {};
    std::string compression = "";
    std::string transform = "";
};

template<>
struct Parameter< Operation::EXTEND_DATASET > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        name(p.name), extent(p.extent) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::EXTEND_DATASET >(*this));
    }

    std::string name = "";
    Extent extent = {};
};

template<>
struct Parameter< Operation::OPEN_DATASET > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        name(p.name), dtype(p.dtype), extent(p.extent) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::OPEN_DATASET >(*this));
    }

    std::string name = "";
    std::shared_ptr< Datatype > dtype
            = std::make_shared< Datatype >();
    std::shared_ptr< Extent > extent
            = std::make_shared< Extent >();
};

template<>
struct Parameter< Operation::DELETE_DATASET > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), name(p.name) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::DELETE_DATASET >(*this));
    }

    std::string name = "";
};

template<>
struct Parameter< Operation::WRITE_DATASET > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        extent(p.extent), offset(p.offset), dtype(p.dtype),
        data(p.data) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::WRITE_DATASET >(*this));
    }

    Extent extent = {};
    Offset offset = {};
    Datatype dtype = Datatype::UNDEFINED;
    std::shared_ptr< void const > data = nullptr;
};

template<>
struct Parameter< Operation::READ_DATASET > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        extent(p.extent), offset(p.offset), dtype(p.dtype),
        data(p.data) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::READ_DATASET >(*this));
    }

    Extent extent = {};
    Offset offset = {};
    Datatype dtype = Datatype::UNDEFINED;
    std::shared_ptr< void > data = nullptr;
};

template<>
struct Parameter< Operation::LIST_DATASETS > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        datasets(p.datasets) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::LIST_DATASETS >(*this));
    }

    std::shared_ptr< std::vector< std::string > > datasets
            = std::make_shared< std::vector< std::string > >();
};

template<>
struct Parameter< Operation::DELETE_ATT > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(), name(p.name) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::DELETE_ATT >(*this));
    }

    std::string name = "";
};

template<>
struct Parameter< Operation::WRITE_ATT > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        name(p.name), dtype(p.dtype), resource(p.resource) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::WRITE_ATT >(*this));
    }

    std::string name = "";
    Datatype dtype = Datatype::UNDEFINED;
    Attribute::resource resource;
};

template<>
struct Parameter< Operation::READ_ATT > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        name(p.name), dtype(p.dtype), resource(p.resource) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::READ_ATT >(*this));
    }

    std::string name = "";
    std::shared_ptr< Datatype > dtype
            = std::make_shared< Datatype >();
    std::shared_ptr< Attribute::resource > resource
            = std::make_shared< Attribute::resource >();
};

template<>
struct Parameter< Operation::LIST_ATTS > : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const & p) : AbstractParameter(),
        attributes(p.attributes) {};

    std::unique_ptr< AbstractParameter >
    clone() const override
    {
        return std::unique_ptr< AbstractParameter >(
            new Parameter< Operation::LIST_ATTS >(*this));
    }

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
    explicit IOTask(Writable* w,
           Parameter< op > const & p)
            : writable{w},
              operation{op},
              parameter{p.clone()}
    { }

    template< Operation op >
    explicit IOTask(Attributable* a,
           Parameter< op > const & p)
            : writable{getWritable(a)},
              operation{op},
              parameter{p.clone()}
    { }

    explicit IOTask(IOTask const & other) :
        writable{other.writable},
        operation{other.operation},
        parameter{other.parameter}
    {}

    Writable* writable;
    Operation operation;
    std::shared_ptr< AbstractParameter > parameter;
};  // IOTask
} // namespace openPMD
