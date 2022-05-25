/* Copyright 2017-2021 Fabian Koller
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

#include "openPMD/ChunkInfo.hpp"
#include "openPMD/Dataset.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/Streaming.hpp"
#include "openPMD/auxiliary/Export.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attribute.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace openPMD
{
class AttributableInterface;
class Writable;

Writable *getWritable(AttributableInterface *);

/** Type of IO operation between logical and persistent data.
 */
OPENPMDAPI_EXPORT_ENUM_CLASS(Operation){
    CREATE_FILE,     OPEN_FILE,      CLOSE_FILE,    DELETE_FILE,

    CREATE_PATH,     CLOSE_PATH,     OPEN_PATH,     DELETE_PATH,
    LIST_PATHS,

    CREATE_DATASET,  EXTEND_DATASET, OPEN_DATASET,  DELETE_DATASET,
    WRITE_DATASET,   READ_DATASET,   LIST_DATASETS, GET_BUFFER_VIEW,

    DELETE_ATT,      WRITE_ATT,      READ_ATT,      LIST_ATTS,

    ADVANCE,
    AVAILABLE_CHUNKS //!< Query chunks that can be loaded in a dataset
}; // note: if you change the enum members here, please update
   // docs/source/dev/design.rst

namespace internal
{
    /*
     * The returned strings are compile-time constants, so no worries about
     * pointer validity.
     */
    std::string operationAsString(Operation);
} // namespace internal

struct OPENPMDAPI_EXPORT AbstractParameter
{
    virtual ~AbstractParameter() = default;
    AbstractParameter() = default;
    // AbstractParameter(AbstractParameter&&) = default;

    // avoid object slicing
    AbstractParameter(const AbstractParameter &) = delete;
    AbstractParameter &operator=(const AbstractParameter &) = delete;
    virtual std::unique_ptr<AbstractParameter> clone() const = 0;
};

/** @brief Typesafe description of all required arguments for a specified
 * Operation.
 *
 * @note    Input operations (i.e. ones that transfer data from persistent files
 *          to logical representations in openPMD-api) use shared pointers to
 *          indicate shared ownership of the resource. The pointer will only be
 *          valid after the Operation has completed.
 * @tparam  Operation   Type of Operation to be executed.
 */
template <Operation>
struct OPENPMDAPI_EXPORT Parameter : public AbstractParameter
{
    Parameter() = delete;
    Parameter(Parameter const &) = delete;
    Parameter(Parameter &&) = delete;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CREATE_FILE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter(), name(p.name), encoding(p.encoding)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CREATE_FILE>(*this));
    }

    std::string name = "";
    IterationEncoding encoding = IterationEncoding::groupBased;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::OPEN_FILE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter(), name(p.name), encoding(p.encoding)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::OPEN_FILE>(*this));
    }

    std::string name = "";
    /*
     * The backends might need to ensure availability of certain features
     * for some iteration encodings, e.g. availability of ADIOS steps for
     * variableBased encoding.
     */
    IterationEncoding encoding = IterationEncoding::groupBased;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CLOSE_FILE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &) : AbstractParameter()
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CLOSE_FILE>(*this));
    }
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::DELETE_FILE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), name(p.name)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::DELETE_FILE>(*this));
    }

    std::string name = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CREATE_PATH>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), path(p.path)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CREATE_PATH>(*this));
    }

    std::string path = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CLOSE_PATH>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &) : AbstractParameter()
    {}

    Parameter &operator=(Parameter const &)
    {
        return *this;
    }

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CLOSE_PATH>(*this));
    }
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::OPEN_PATH>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), path(p.path)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::OPEN_PATH>(*this));
    }

    std::string path = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::DELETE_PATH>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), path(p.path)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::DELETE_PATH>(*this));
    }

    std::string path = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::LIST_PATHS>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), paths(p.paths)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::LIST_PATHS>(*this));
    }

    std::shared_ptr<std::vector<std::string> > paths =
        std::make_shared<std::vector<std::string> >();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CREATE_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter()
        , name(p.name)
        , extent(p.extent)
        , dtype(p.dtype)
        , chunkSize(p.chunkSize)
        , compression(p.compression)
        , transform(p.transform)
        , options(p.options)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CREATE_DATASET>(*this));
    }

    std::string name = "";
    Extent extent = {};
    Datatype dtype = Datatype::UNDEFINED;
    Extent chunkSize = {};
    std::string compression = "";
    std::string transform = "";
    std::string options = "{}";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::EXTEND_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), extent(p.extent)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::EXTEND_DATASET>(*this));
    }

    Extent extent = {};
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::OPEN_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter(), name(p.name), dtype(p.dtype), extent(p.extent)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::OPEN_DATASET>(*this));
    }

    std::string name = "";
    std::shared_ptr<Datatype> dtype = std::make_shared<Datatype>();
    std::shared_ptr<Extent> extent = std::make_shared<Extent>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::DELETE_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), name(p.name)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::DELETE_DATASET>(*this));
    }

    std::string name = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::WRITE_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter<Operation::WRITE_DATASET> const &p)
        : AbstractParameter()
        , extent(p.extent)
        , offset(p.offset)
        , dtype(p.dtype)
        , data(p.data)
    {}

    Parameter &operator=(const Parameter &p)
    {
        this->extent = p.extent;
        this->offset = p.offset;
        this->dtype = p.dtype;
        this->data = p.data;
        return *this;
    }

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::WRITE_DATASET>(*this));
    }

    Extent extent = {};
    Offset offset = {};
    Datatype dtype = Datatype::UNDEFINED;
    std::shared_ptr<void const> data = nullptr;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::READ_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter<Operation::READ_DATASET> const &p)
        : AbstractParameter()
        , extent(p.extent)
        , offset(p.offset)
        , dtype(p.dtype)
        , data(p.data)
    {}

    Parameter &operator=(const Parameter &p)
    {
        this->extent = p.extent;
        this->offset = p.offset;
        this->dtype = p.dtype;
        this->data = p.data;
        return *this;
    }

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::READ_DATASET>(*this));
    }

    Extent extent = {};
    Offset offset = {};
    Datatype dtype = Datatype::UNDEFINED;
    std::shared_ptr<void> data = nullptr;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::LIST_DATASETS>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), datasets(p.datasets)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::LIST_DATASETS>(*this));
    }

    std::shared_ptr<std::vector<std::string> > datasets =
        std::make_shared<std::vector<std::string> >();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::GET_BUFFER_VIEW>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter()
        , offset(p.offset)
        , extent(p.extent)
        , dtype(p.dtype)
        , update(p.update)
        , out(p.out)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::GET_BUFFER_VIEW>(*this));
    }

    // in parameters
    Offset offset;
    Extent extent;
    Datatype dtype = Datatype::UNDEFINED;
    bool update = false;
    // out parameters
    struct OutParameters
    {
        bool backendManagedBuffer = false;
        unsigned viewIndex = 0;
        void *ptr = nullptr;
    };
    std::shared_ptr<OutParameters> out = std::make_shared<OutParameters>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::DELETE_ATT>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), name(p.name)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::DELETE_ATT>(*this));
    }

    std::string name = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::WRITE_ATT>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter()
        , name(p.name)
        , dtype(p.dtype)
        , resource(p.resource)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::WRITE_ATT>(*this));
    }

    std::string name = "";
    Datatype dtype = Datatype::UNDEFINED;
    Attribute::resource resource;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::READ_ATT>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter()
        , name(p.name)
        , dtype(p.dtype)
        , resource(p.resource)
    {}

    Parameter &operator=(const Parameter &p)
    {
        this->name = p.name;
        this->dtype = p.dtype;
        this->resource = p.resource;
        return *this;
    }

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::READ_ATT>(*this));
    }

    std::string name = "";
    std::shared_ptr<Datatype> dtype = std::make_shared<Datatype>();
    std::shared_ptr<Attribute::resource> resource =
        std::make_shared<Attribute::resource>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::LIST_ATTS>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter(), attributes(p.attributes)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::LIST_ATTS>(*this));
    }

    std::shared_ptr<std::vector<std::string> > attributes =
        std::make_shared<std::vector<std::string> >();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::ADVANCE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p)
        : AbstractParameter(), mode(p.mode), status(p.status)
    {}

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::ADVANCE>(*this));
    }

    //! input parameter
    AdvanceMode mode;
    //! output parameter
    std::shared_ptr<AdvanceStatus> status =
        std::make_shared<AdvanceStatus>(AdvanceStatus::OK);
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::AVAILABLE_CHUNKS>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter const &p) : AbstractParameter(), chunks(p.chunks)
    {}

    Parameter &operator=(Parameter const &p)
    {
        chunks = p.chunks;
        return *this;
    }

    std::unique_ptr<AbstractParameter> clone() const override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::AVAILABLE_CHUNKS>(*this));
    }

    // output parameter
    std::shared_ptr<ChunkTable> chunks = std::make_shared<ChunkTable>();
};

/** @brief Self-contained description of a single IO operation.
 *
 * Contained are
 * 1) the parameters to
 * 2) a single atomic file Operation on the
 * 3) concrete Writable object corresponding to both a local representation in
 *    openPMD-api and a persistent object in a file on disk
 */
class OPENPMDAPI_EXPORT IOTask
{
public:
    /** Constructor for self-contained description of single IO operation.
     *
     * @tparam  op  Type of Operation to be executed.
     * @param   w   Writable indicating the location of the object being
     * operated on.
     * @param   p   Parameter object supplying all required input and/or output
     * parameters to the operation.
     */
    template <Operation op>
    explicit IOTask(Writable *w, Parameter<op> const &p)
        : writable{w}, operation{op}, parameter{p.clone()}
    {}

    template <Operation op>
    explicit IOTask(AttributableInterface *a, Parameter<op> const &p)
        : writable{getWritable(a)}, operation{op}, parameter{p.clone()}
    {}

    explicit IOTask(IOTask const &other)
        : writable{other.writable}
        , operation{other.operation}
        , parameter{other.parameter}
    {}

    IOTask &operator=(IOTask const &other)
    {
        writable = other.writable;
        operation = other.operation;
        parameter = other.parameter;
        return *this;
    }

    Writable *writable;
    Operation operation;
    std::shared_ptr<AbstractParameter> parameter;
}; // IOTask
} // namespace openPMD
