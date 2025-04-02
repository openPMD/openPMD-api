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
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/ParsePreference.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace openPMD
{
class Attributable;
class Writable;
namespace json
{
    class JsonMatcher;
}

Writable *getWritable(Attributable *);

/** Type of IO operation between logical and persistent data.
 */
OPENPMDAPI_EXPORT_ENUM_CLASS(Operation){
    CREATE_FILE,
    CHECK_FILE,
    OPEN_FILE,
    CLOSE_FILE,
    DELETE_FILE,

    CREATE_PATH,
    CLOSE_PATH,
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
    GET_BUFFER_VIEW,

    DELETE_ATT,
    WRITE_ATT,
    READ_ATT,
    READ_ATT_ALLSTEPS,
    LIST_ATTS,

    ADVANCE,
    AVAILABLE_CHUNKS, //!< Query chunks that can be loaded in a dataset
    DEREGISTER, //!< Inform the backend that an object has been deleted.
    TOUCH, //!< tell the backend that the file is to be considered active
    SET_WRITTEN //!< tell backend to consider a file written / not written
}; // note: if you change the enum members here, please update
   // docs/source/dev/design.rst

namespace internal
{
    /*
     * The returned strings are compile-time constants, so no worries about
     * pointer validity.
     */
    OPENPMDAPI_EXPORT std::string operationAsString(Operation);
} // namespace internal

struct OPENPMDAPI_EXPORT AbstractParameter
{
    virtual ~AbstractParameter() = default;
    AbstractParameter() = default;

    virtual std::unique_ptr<AbstractParameter> to_heap() && = 0;

    /** Warn about unused JSON paramters
     *
     * Template parameter so we don't have to include the JSON lib here.
     * This function is useful for the createDataset() methods in,
     * IOHandlerImpl's, so putting that here is the simplest way to make it
     * available for them. */
    template <typename TracingJSON>
    static void warnUnusedParameters(
        TracingJSON &,
        std::string const &currentBackendName,
        std::string const &warningMessage);

    // Used as a tag in some constructors to make callsites explicitly
    // acknowledge that joined dimensions will not be automatically resolved
    struct I_dont_want_to_use_joined_dimensions_t
    {};
    constexpr static I_dont_want_to_use_joined_dimensions_t
        I_dont_want_to_use_joined_dimensions{};

protected:
    // avoid object slicing
    // by allow only child classes to use these things for defining their own
    // copy/move constructors/assignment operators
    AbstractParameter(const AbstractParameter &) = default;
    AbstractParameter &operator=(const AbstractParameter &) = default;
    AbstractParameter(AbstractParameter &&) = default;
    AbstractParameter &operator=(AbstractParameter &&) = default;
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
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CREATE_FILE>(std::move(*this)));
    }

    std::string name = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CHECK_FILE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CHECK_FILE>(std::move(*this)));
    }

    std::string name = "";
    enum class FileExists
    {
        DontKnow,
        Yes,
        No
    };
    std::shared_ptr<FileExists> fileExists =
        std::make_shared<FileExists>(FileExists::DontKnow);
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::OPEN_FILE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::OPEN_FILE>(std::move(*this)));
    }

    // Needed for reopening files in file-based Iteration encoding when using
    // R/W-mode in ADIOS2. Files can only be opened for reading XOR writing,
    // so R/W mode in file-based encoding can only operate at the granularity
    // of files in ADIOS2. The frontend needs to tell us if we should reopen
    // a file for continued reading (WasFoundOnDisk) or for continued writing
    // (WasCreatedByUs).
    enum class Reopen
    {
        WasCreatedByUs,
        WasFoundOnDisk,
        NoReopen
    };

    std::string name = "";
    Reopen reopen = Reopen::NoReopen;
    using ParsePreference = internal::ParsePreference;
    std::shared_ptr<ParsePreference> out_parsePreference =
        std::make_shared<ParsePreference>(ParsePreference::UpFront);
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CLOSE_FILE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CLOSE_FILE>(std::move(*this)));
    }
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::DELETE_FILE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::DELETE_FILE>(std::move(*this)));
    }

    std::string name = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CREATE_PATH>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CREATE_PATH>(std::move(*this)));
    }

    std::string path = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CLOSE_PATH>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CLOSE_PATH>(std::move(*this)));
    }
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::OPEN_PATH>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::OPEN_PATH>(std::move(*this)));
    }

    std::string path = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::DELETE_PATH>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::DELETE_PATH>(std::move(*this)));
    }

    std::string path = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::LIST_PATHS>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::LIST_PATHS>(std::move(*this)));
    }

    std::shared_ptr<std::vector<std::string>> paths =
        std::make_shared<std::vector<std::string>>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::CREATE_DATASET>
    : public AbstractParameter
{
    Parameter(Dataset const &ds)
        : extent(ds.extent)
        , dtype(ds.dtype)
        , options(ds.options)
        , joinedDimension(ds.joinedDimension())
    {}

    // default constructor, but callsites need to explicitly acknowledge that
    // joined dimensions will not be automatically configured when using it
    Parameter(I_dont_want_to_use_joined_dimensions_t)
    {}
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::CREATE_DATASET>(std::move(*this)));
    }

    std::string name = "";
    Extent extent = {};
    Datatype dtype = Datatype::UNDEFINED;
    std::string options = "{}";
    std::optional<size_t> joinedDimension;

    template <typename TracingJSON>
    TracingJSON compileJSONConfig(
        Writable const *writable,
        json::JsonMatcher &,
        std::string const &backendName) const;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::EXTEND_DATASET>
    : public AbstractParameter
{
    Parameter(Extent e) : joinedDimension(Dataset::joinedDimension(e))
    {
        this->extent = std::move(e);
    }

    // default constructor, but callsites need to explicitly acknowledge that
    // joined dimensions will not be automatically configured when using it
    Parameter(I_dont_want_to_use_joined_dimensions_t)
    {}
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::EXTEND_DATASET>(std::move(*this)));
    }

    Extent extent = {};
    std::optional<size_t> joinedDimension;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::OPEN_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::OPEN_DATASET>(std::move(*this)));
    }

    template <typename TracingJSON>
    static TracingJSON compileJSONConfig(
        Writable const *writable,
        json::JsonMatcher &,
        std::string const &backendName);

    std::string name = "";
    std::shared_ptr<Datatype> dtype = std::make_shared<Datatype>();
    std::shared_ptr<Extent> extent = std::make_shared<Extent>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::DELETE_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::DELETE_DATASET>(std::move(*this)));
    }

    std::string name = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::WRITE_DATASET>
    : public AbstractParameter
{
    Parameter() = default;

    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = delete;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = delete;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::WRITE_DATASET>(std::move(*this)));
    }

    Extent extent = {};
    Offset offset = {};
    Datatype dtype = Datatype::UNDEFINED;
    auxiliary::WriteBuffer data;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::READ_DATASET>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::READ_DATASET>(std::move(*this)));
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
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::LIST_DATASETS>(std::move(*this)));
    }

    std::shared_ptr<std::vector<std::string>> datasets =
        std::make_shared<std::vector<std::string>>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::GET_BUFFER_VIEW>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::GET_BUFFER_VIEW>(std::move(*this)));
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
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::DELETE_ATT>(std::move(*this)));
    }

    std::string name = "";
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::WRITE_ATT>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::WRITE_ATT>(std::move(*this)));
    }

    std::string name = "";
    Datatype dtype = Datatype::UNDEFINED;
    /*
     * If true, this attribute changes across IO steps.
     * It should only be written in backends that support IO steps,
     * otherwise writing should be skipped.
     * The frontend is responsible for handling both situations.
     */
    enum class ChangesOverSteps
    {
        No,
        Yes,
        IfPossible
    };
    ChangesOverSteps changesOverSteps = ChangesOverSteps::No;
    Attribute::resource resource;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::READ_ATT>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::READ_ATT>(std::move(*this)));
    }

    std::string name = "";
    std::shared_ptr<Datatype> dtype = std::make_shared<Datatype>();
    std::shared_ptr<Attribute::resource> resource =
        std::make_shared<Attribute::resource>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::READ_ATT_ALLSTEPS>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::READ_ATT_ALLSTEPS>(std::move(*this)));
    }

    std::string name = "";
    std::shared_ptr<Datatype> dtype = std::make_shared<Datatype>();

    struct to_vector_type
    {
        template <typename T>
        using type = std::vector<T>;
    };
    // std::variant<std::vector<T_1>, std::vector<T_2>, ...>
    // for all T_i in openPMD::Datatype.
    using result_type = typename auxiliary::detail::
        map_variant<to_vector_type, Attribute::resource>::type;

    std::shared_ptr<result_type> resource = std::make_shared<result_type>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::LIST_ATTS>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::LIST_ATTS>(std::move(*this)));
    }

    std::shared_ptr<std::vector<std::string>> attributes =
        std::make_shared<std::vector<std::string>>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::ADVANCE>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::ADVANCE>(std::move(*this)));
    }

    struct StepSelection
    {
        std::optional<size_t> step;
    };

    // input parameters
    /**
     * AdvanceMode: Is one of BeginStep/EndStep. Used during writing and in
     *      linear read mode to step sequentially through steps.
     * StepSelection: Used in random-access read mode, jump to the specified
     *      step. Can be nullopt in order to reset the backend to read
     *      step-agnostically, e.g. for reading global datasets such as
     *      /rankTable.
     */
    std::variant<AdvanceMode, StepSelection> mode;
    bool isThisStepMandatory = false;
    // output parameter
    std::shared_ptr<AdvanceStatus> status =
        std::make_shared<AdvanceStatus>(AdvanceStatus::OK);
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::AVAILABLE_CHUNKS>
    : public AbstractParameter
{
    Parameter() = default;
    Parameter(Parameter &&) = default;
    Parameter(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;
    Parameter &operator=(Parameter const &) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::unique_ptr<AbstractParameter>(
            new Parameter<Operation::AVAILABLE_CHUNKS>(std::move(*this)));
    }

    // output parameter
    std::shared_ptr<ChunkTable> chunks = std::make_shared<ChunkTable>();
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::DEREGISTER>
    : public AbstractParameter
{
    Parameter(void const *ptr_in) : former_parent(ptr_in)
    {}

    Parameter(Parameter const &) = default;
    Parameter(Parameter &&) = default;

    Parameter &operator=(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::make_unique<Parameter<Operation::DEREGISTER>>(
            std::move(*this));
    }

    // Just for verbose logging.
    void const *former_parent = nullptr;
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::TOUCH> : public AbstractParameter
{
    explicit Parameter() = default;

    Parameter(Parameter const &) = default;
    Parameter(Parameter &&) = default;

    Parameter &operator=(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::make_unique<Parameter<Operation::TOUCH>>(std::move(*this));
    }
};

template <>
struct OPENPMDAPI_EXPORT Parameter<Operation::SET_WRITTEN>
    : public AbstractParameter
{
    explicit Parameter() = default;

    Parameter(Parameter const &) = default;
    Parameter(Parameter &&) = default;

    Parameter &operator=(Parameter const &) = default;
    Parameter &operator=(Parameter &&) = default;

    std::unique_ptr<AbstractParameter> to_heap() && override
    {
        return std::make_unique<Parameter<Operation::SET_WRITTEN>>(
            std::move(*this));
    }

    bool target_status = false;
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
    explicit IOTask(Writable *w, Parameter<op> p)
        : writable{w}, operation{op}, parameter{std::move(p).to_heap()}
    {}

    template <Operation op>
    explicit IOTask(Attributable *a, Parameter<op> p)
        : writable{getWritable(a)}
        , operation{op}
        , parameter{std::move(p).to_heap()}
    {}

    IOTask(IOTask const &other);
    IOTask(IOTask &&other) noexcept;
    IOTask &operator=(IOTask const &other);
    IOTask &operator=(IOTask &&other) noexcept;

    Writable *writable;
    Operation operation;
    std::shared_ptr<AbstractParameter> parameter;
}; // IOTask
} // namespace openPMD
