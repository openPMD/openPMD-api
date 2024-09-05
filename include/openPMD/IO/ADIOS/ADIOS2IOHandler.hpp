/* Copyright 2017-2021 Fabian Koller and Franz Poeschel
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

#include "openPMD/Error.hpp"
#include "openPMD/IO/ADIOS/ADIOS2Auxiliary.hpp"
#include "openPMD/IO/ADIOS/ADIOS2FilePosition.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/AbstractIOHandlerImpl.hpp"
#include "openPMD/IO/AbstractIOHandlerImplCommon.hpp"
#include "openPMD/IO/FlushParametersInternal.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/IO/InvalidatableFile.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/ThrowError.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/config.hpp"
#include <stdexcept>

#if openPMD_HAVE_ADIOS2
#include <adios2.h>
#endif
#if openPMD_HAVE_MPI
#include <mpi.h>
#endif
#include <nlohmann/json.hpp>

#include <array>
#include <exception>
#include <future>
#include <iostream>
#include <memory> // shared_ptr
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility> // pair
#include <vector>

namespace openPMD
{
#if openPMD_HAVE_ADIOS2

std::optional<size_t> joinedDimension(adios2::Dims const &dims);

class ADIOS2IOHandler;

namespace detail
{
    template <typename, typename>
    struct DatasetHelper;
    struct GetSpan;
    struct DatasetReader;
    struct AttributeReader;
    struct AttributeWriter;
    struct AttributeReader;
    struct AttributeWriter;
    template <typename>
    struct AttributeTypes;
    struct DatasetOpener;
    struct VariableDefiner;
    template <typename>
    struct DatasetTypes;
    struct WriteDataset;
    class ADIOS2File;
    struct BufferedPut;
    struct BufferedGet;
    struct BufferedAttributeRead;
    struct BufferedAttributeWrite;
    struct RunUniquePtrPut;
} // namespace detail

class ADIOS2IOHandlerImpl
    : public AbstractIOHandlerImplCommon<ADIOS2FilePosition>
{
    template <typename, typename>
    friend struct detail::DatasetHelper;
    friend struct detail::GetSpan;
    friend struct detail::DatasetReader;
    friend struct detail::AttributeReader;
    friend struct detail::AttributeWriter;
    friend struct detail::AttributeReader;
    friend struct detail::AttributeWriter;
    template <typename>
    friend struct detail::AttributeTypes;
    friend struct detail::DatasetOpener;
    friend struct detail::VariableDefiner;
    template <typename>
    friend struct detail::DatasetTypes;
    friend struct detail::WriteDataset;
    friend class detail::ADIOS2File;
    friend struct detail::BufferedAttributeRead;
    friend struct detail::RunUniquePtrPut;

    using UseGroupTable = adios_defs::UseGroupTable;
    using FlushTarget = adios_defs::FlushTarget;

public:
#if openPMD_HAVE_MPI

    ADIOS2IOHandlerImpl(
        AbstractIOHandler *,
        MPI_Comm,
        json::TracingJSON config,
        std::string engineType,
        std::string specifiedExtension);

#endif // openPMD_HAVE_MPI

    explicit ADIOS2IOHandlerImpl(
        AbstractIOHandler *,
        json::TracingJSON config,
        std::string engineType,
        std::string specifiedExtension);

    ~ADIOS2IOHandlerImpl() override;

    std::future<void> flush(internal::ParsedFlushParams &);

    void
    createFile(Writable *, Parameter<Operation::CREATE_FILE> const &) override;

    void checkFile(Writable *, Parameter<Operation::CHECK_FILE> &) override;

    // MPI Collective
    bool checkFile(std::string fullFilePath) const;

    void
    createPath(Writable *, Parameter<Operation::CREATE_PATH> const &) override;

    void createDataset(
        Writable *, Parameter<Operation::CREATE_DATASET> const &) override;

    void extendDataset(
        Writable *, Parameter<Operation::EXTEND_DATASET> const &) override;

    void openFile(Writable *, Parameter<Operation::OPEN_FILE> &) override;

    void
    closeFile(Writable *, Parameter<Operation::CLOSE_FILE> const &) override;

    void openPath(Writable *, Parameter<Operation::OPEN_PATH> const &) override;

    void
    closePath(Writable *, Parameter<Operation::CLOSE_PATH> const &) override;

    void openDataset(Writable *, Parameter<Operation::OPEN_DATASET> &) override;

    void
    deleteFile(Writable *, Parameter<Operation::DELETE_FILE> const &) override;

    void
    deletePath(Writable *, Parameter<Operation::DELETE_PATH> const &) override;

    void deleteDataset(
        Writable *, Parameter<Operation::DELETE_DATASET> const &) override;

    void deleteAttribute(
        Writable *, Parameter<Operation::DELETE_ATT> const &) override;

    void
    writeDataset(Writable *, Parameter<Operation::WRITE_DATASET> &) override;

    void writeAttribute(
        Writable *, Parameter<Operation::WRITE_ATT> const &) override;

    void readDataset(Writable *, Parameter<Operation::READ_DATASET> &) override;

    void
    getBufferView(Writable *, Parameter<Operation::GET_BUFFER_VIEW> &) override;

    void readAttribute(Writable *, Parameter<Operation::READ_ATT> &) override;

    void listPaths(Writable *, Parameter<Operation::LIST_PATHS> &) override;

    void
    listDatasets(Writable *, Parameter<Operation::LIST_DATASETS> &) override;

    void listAttributes(
        Writable *, Parameter<Operation::LIST_ATTS> &parameters) override;

    void advance(Writable *, Parameter<Operation::ADVANCE> &) override;

    void availableChunks(
        Writable *, Parameter<Operation::AVAILABLE_CHUNKS> &) override;

    void
    deregister(Writable *, Parameter<Operation::DEREGISTER> const &) override;

    void touch(Writable *, Parameter<Operation::TOUCH> const &) override;

    /**
     * @brief The ADIOS2 access type to chose for Engines opened
     * within this instance.
     */
    adios2::Mode adios2AccessMode(std::string const &fullPath);

    FlushTarget m_flushTarget = FlushTarget::Disk;

private:
    adios2::ADIOS m_ADIOS;
#if openPMD_HAVE_MPI
    std::optional<MPI_Comm> m_communicator;
#endif
    /**
     * The ADIOS2 engine type, to be passed to adios2::IO::SetEngine
     */
    std::string m_engineType;
    std::optional<std::string> m_realEngineType;

    inline std::string const &realEngineType() const
    {
        if (m_realEngineType.has_value())
        {
            return *m_realEngineType;
        }
        else
        {
            return m_engineType;
        }
    }
    inline std::string &realEngineType()
    {
        return const_cast<std::string &>(
            static_cast<ADIOS2IOHandlerImpl const *>(this)->realEngineType());
    }
    inline void pretendEngine(std::string facade_engine)
    {
        if (!m_realEngineType.has_value())
        {
            m_realEngineType = std::move(m_engineType);
        }
        m_engineType = std::move(facade_engine);
    }
    /*
     * The filename extension specified by the user.
     */
    std::string m_userSpecifiedExtension;

    /*
     * Empty option: No choice about the group table has been explicitly made,
     * use default.
     */
    std::optional<UseGroupTable> m_useGroupTable;

    enum class UseSpan : char
    {
        Yes,
        No,
        Auto
    };

    UseSpan m_useSpanBasedPutByDefault = UseSpan::Auto;

    enum class ModifiableAttributes : char
    {
        Yes,
        No,
        Unspecified
    };

    ModifiableAttributes m_modifiableAttributes =
        ModifiableAttributes::Unspecified;

    inline UseGroupTable useGroupTable() const
    {
        if (!m_useGroupTable.has_value())
        {
            return UseGroupTable::No;
        }
        return m_useGroupTable.value();
    }

    bool m_writeAttributesFromThisRank = true;

    struct ParameterizedOperator
    {
        adios2::Operator op;
        adios2::Params params;
    };

    std::vector<ParameterizedOperator> defaultOperators;

    json::TracingJSON m_config;
    static json::TracingJSON nullvalue;

    template <typename Callback>
    void
    init(json::TracingJSON config, Callback &&callbackWriteAttributesFromRank);

    template <typename Key>
    json::TracingJSON config(Key &&key, json::TracingJSON &cfg)
    {
        if (cfg.json().is_object() && cfg.json().contains(key))
        {
            return cfg[key];
        }
        else
        {
            return nullvalue;
        }
    }

    template <typename Key>
    json::TracingJSON config(Key &&key)
    {
        return config<Key>(std::forward<Key>(key), m_config);
    }

    /**
     *
     * @param config The top-level of the ADIOS2 configuration JSON object
     * with operators to be found under dataset.operators
     * @return first parameter: the operators, second parameters: whether
     * operators have been configured
     */
    std::optional<std::vector<ParameterizedOperator>>
    getOperators(json::TracingJSON config);

    // use m_config
    std::optional<std::vector<ParameterizedOperator>> getOperators();

    std::string fileSuffix(bool verbose = true) const;

    /*
     * We need to give names to IO objects. These names are irrelevant
     * within this application, since:
     * 1) The name of the file written to is decided by the opened Engine's
     *    name.
     * 2) The IOs are managed by the unordered_map m_fileData, so we do not
     *    need the ADIOS2 internal management.
     * Since within one m_ADIOS object, the same IO name cannot be used more
     * than once, we ensure different names by using the name counter.
     * This allows to overwrite a file later without error.
     */
    int nameCounter{0};

    /*
     * IO-heavy actions are deferred to a later point. This map stores for
     * each open file (identified by an InvalidatableFile object) an object
     * that manages IO-heavy actions, as well as its ADIOS2 objects, i.e.
     * IO and Engine object.
     * Not to be accessed directly, use getFileData().
     */
    std::unordered_map<InvalidatableFile, std::unique_ptr<detail::ADIOS2File>>
        m_fileData;

    std::map<std::string, adios2::Operator> m_operators;

    // Overrides from AbstractIOHandlerImplCommon.

    std::string
        filePositionToString(std::shared_ptr<ADIOS2FilePosition>) override;

    std::shared_ptr<ADIOS2FilePosition> extendFilePosition(
        std::shared_ptr<ADIOS2FilePosition> const &pos,
        std::string extend) override;

    // Helper methods.

    std::optional<adios2::Operator>
    getCompressionOperator(std::string const &compression);

    /*
     * The name of the ADIOS2 variable associated with this Writable.
     * To be used for Writables that represent a dataset.
     */
    std::string nameOfVariable(Writable *writable);

    /**
     * @brief nameOfAttribute
     * @param writable The Writable at whose level the attribute lies.
     * @param attribute The openPMD name of the attribute.
     * @return The ADIOS2 name of the attribute, consisting of
     * the variable that the attribute is associated with
     * (possibly the empty string, representing no variable)
     * and the actual name.
     */
    std::string nameOfAttribute(Writable *writable, std::string attribute);

    /*
     * Figure out whether the Writable corresponds with a
     * group or a dataset.
     */
    GroupOrDataset groupOrDataset(Writable *);

    enum class IfFileNotOpen : bool
    {
        OpenImplicitly,
        ThrowError
    };

    detail::ADIOS2File &
    getFileData(InvalidatableFile const &file, IfFileNotOpen);

    void dropFileData(InvalidatableFile const &file);

    /*
     * Prepare a variable that already exists for an IO
     * operation, including:
     * (1) checking that its datatype matches T.
     * (2) the offset and extent match the variable's shape
     * (3) setting the offset and extent (ADIOS lingo: start
     *     and count)
     */
    template <typename T>
    adios2::Variable<T> verifyDataset(
        Offset const &offset,
        Extent const &extent,
        adios2::IO &IO,
        std::string const &varName)
    {
        {
            auto requiredType = adios2::GetType<T>();
            auto actualType = IO.VariableType(varName);

            if (requiredType != actualType)
            {
                std::stringstream errorMessage;
                errorMessage << "Trying to access a dataset with wrong type "
                                "(trying to access dataset with type '"
                             << requiredType << "', but has type '"
                             << actualType << "')";
                throw error::ReadError(
                    error::AffectedObject::Dataset,
                    error::Reason::UnexpectedContent,
                    "ADIOS2",
                    errorMessage.str());
            };
        }
        adios2::Variable<T> var = IO.InquireVariable<T>(varName);
        if (!var.operator bool())
        {

            throw std::runtime_error(
                "[ADIOS2] Internal error: Failed opening ADIOS2 variable.");
        }
        // TODO leave this check to ADIOS?
        adios2::Dims shape = var.Shape();
        auto actualDim = shape.size();
        {
            auto requiredDim = extent.size();
            if (requiredDim != actualDim)
            {
                throw error::ReadError(
                    error::AffectedObject::Dataset,
                    error::Reason::UnexpectedContent,
                    "ADIOS2",
                    "Trying to access a dataset with wrong dimensionality "
                    "(trying to access dataset with dimensionality " +
                        std::to_string(requiredDim) +
                        ", but has dimensionality " +
                        std::to_string(actualDim) + ")");
            }
        }
        auto joinedDim = joinedDimension(shape);
        if (joinedDim.has_value())
        {
            if (!offset.empty())
            {
                throw std::runtime_error(
                    "[ADIOS2] Offset must be an empty vector in case of joined "
                    "array.");
            }
            for (unsigned int i = 0; i < actualDim; i++)
            {
                if (*joinedDim != i && extent[i] != shape[i])
                {
                    throw std::runtime_error(
                        "[ADIOS2] store_chunk extent of non-joined dimensions "
                        "must be equivalent to the total extent.");
                }
            }
        }
        else
        {
            for (unsigned int i = 0; i < actualDim; i++)
            {
                if (!(joinedDim.has_value() && *joinedDim == i) &&
                    offset[i] + extent[i] > shape[i])
                {
                    throw std::runtime_error(
                        "[ADIOS2] Dataset access out of bounds.");
                }
            }
        }

        var.SetSelection(
            {adios2::Dims(offset.begin(), offset.end()),
             adios2::Dims(extent.begin(), extent.end())});
        return var;
    }

    struct
    {
        bool noGroupBased = false;
        bool blosc2bp5 = false;
    } printedWarningsAlready;
}; // ADIOS2IOHandlerImpl

namespace detail
{
    // Helper structs for calls to the switchType function

    template <typename T>
    inline constexpr bool IsUnsupportedComplex_v =
        std::is_same_v<T, std::complex<long double>> ||
        std::is_same_v<T, std::vector<std::complex<long double>>>;

    struct AttributeReader
    {
        template <typename T>
        static Datatype call(
            ADIOS2IOHandlerImpl &,
            adios2::IO &IO,
            std::string name,
            Attribute::resource &resource);

        template <int n, typename... Params>
        static Datatype call(Params &&...);
    };

    struct AttributeWriter
    {
        template <typename T>
        static void call(
            ADIOS2IOHandlerImpl *impl,
            Writable *writable,
            const Parameter<Operation::WRITE_ATT> &parameters);

        template <int n, typename... Params>
        static void call(Params &&...);
    };

    struct DatasetOpener
    {
        template <typename T>
        static void call(
            ADIOS2IOHandlerImpl *impl,
            InvalidatableFile const &,
            std::string const &varName,
            Parameter<Operation::OPEN_DATASET> &parameters);

        static constexpr char const *errorMsg = "ADIOS2: openDataset()";
    };

    struct VariableDefiner
    {
        /**
         * @brief Define a Variable of type T within the passed IO.
         *
         * @param IO The adios2::IO object within which to define the
         *           variable. The variable can later be retrieved from
         *           the IO using the passed name.
         * @param name As in adios2::IO::DefineVariable
         * @param compressions ADIOS2 operators, including an arbitrary
         *                     number of parameters, to be added to the
         *                     variable upon definition.
         * @param shape As in adios2::IO::DefineVariable
         * @param start As in adios2::IO::DefineVariable
         * @param count As in adios2::IO::DefineVariable
         * @param constantDims As in adios2::IO::DefineVariable
         */
        template <typename T>
        static void call(
            adios2::IO &IO,
            std::string const &name,
            std::vector<ADIOS2IOHandlerImpl::ParameterizedOperator> const
                &compressions,
            adios2::Dims const &shape = adios2::Dims(),
            adios2::Dims const &start = adios2::Dims(),
            adios2::Dims const &count = adios2::Dims(),
            bool const constantDims = false);

        static constexpr char const *errorMsg = "ADIOS2: defineVariable()";
    };

    struct RetrieveBlocksInfo
    {
        template <typename T>
        static void call(
            Parameter<Operation::AVAILABLE_CHUNKS> &params,
            adios2::IO &IO,
            adios2::Engine &engine,
            std::string const &varName,
            bool allSteps);

        template <int n, typename... Params>
        static void call(Params &&...);
    };

    // Helper structs to help distinguish valid attribute/variable
    // datatypes from invalid ones

    /*
     * This struct's purpose is to have specialisations
     * for vector and array types, as well as the boolean
     * type (which is not natively supported by ADIOS).
     */
    template <typename T>
    struct AttributeTypes
    {
        /**
         * @brief Is the attribute given by parameters name and val already
         *        defined exactly in that way within the given IO?
         */
        static bool attributeUnchanged(adios2::IO &IO, std::string name, T val)
        {
            auto attr = IO.InquireAttribute<T>(name);
            if (!attr)
            {
                return false;
            }
            std::vector<T> data = attr.Data();
            if (data.size() != 1)
            {
                return false;
            }
            return data[0] == val;
        }
    };

    template <>
    struct AttributeTypes<std::complex<long double>>
    {
        static bool
        attributeUnchanged(adios2::IO &, std::string, std::complex<long double>)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex "
                "attribute types");
        }
    };

    template <>
    struct AttributeTypes<std::vector<std::complex<long double>>>
    {
        static bool attributeUnchanged(
            adios2::IO &, std::string, std::vector<std::complex<long double>>)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex "
                "vector attribute types");
        }
    };

    template <typename T>
    struct AttributeTypes<std::vector<T>>
    {
        static bool
        attributeUnchanged(adios2::IO &IO, std::string name, std::vector<T> val)
        {
            auto attr = IO.InquireAttribute<T>(name);
            if (!attr)
            {
                return false;
            }
            std::vector<T> data = attr.Data();
            if (data.size() != val.size())
            {
                return false;
            }
            for (size_t i = 0; i < val.size(); ++i)
            {
                if (data[i] != val[i])
                {
                    return false;
                }
            }
            return true;
        }
    };

    template <>
    struct AttributeTypes<std::vector<std::string>>
    {
        static bool attributeUnchanged(
            adios2::IO &IO, std::string name, std::vector<std::string> val)
        {
            auto attr = IO.InquireAttribute<std::string>(name);
            if (!attr)
            {
                return false;
            }
            std::vector<std::string> data = attr.Data();
            if (data.size() != val.size())
            {
                return false;
            }
            for (size_t i = 0; i < val.size(); ++i)
            {
                if (data[i] != val[i])
                {
                    return false;
                }
            }
            return true;
        }
    };

    template <typename T, size_t n>
    struct AttributeTypes<std::array<T, n>>
    {
        static bool attributeUnchanged(
            adios2::IO &IO, std::string name, std::array<T, n> val)
        {
            auto attr = IO.InquireAttribute<T>(name);
            if (!attr)
            {
                return false;
            }
            std::vector<T> data = attr.Data();
            if (data.size() != n)
            {
                return false;
            }
            for (size_t i = 0; i < n; ++i)
            {
                if (data[i] != val[i])
                {
                    return false;
                }
            }
            return true;
        }
    };

    namespace bool_repr
    {
        using rep = detail::bool_representation;

        static constexpr rep toRep(bool b)
        {
            return b ? 1U : 0U;
        }

        static constexpr bool fromRep(rep r)
        {
            return r != 0;
        }
    } // namespace bool_repr

    template <>
    struct AttributeTypes<bool>
    {
        using rep = detail::bool_representation;

        static constexpr rep toRep(bool b)
        {
            return b ? 1U : 0U;
        }

        static constexpr bool fromRep(rep r)
        {
            return r != 0;
        }

        static bool
        attributeUnchanged(adios2::IO &IO, std::string name, bool val)
        {
            auto attr = IO.InquireAttribute<rep>(name);
            if (!attr)
            {
                return false;
            }
            std::vector<rep> data = attr.Data();
            if (data.size() != 1)
            {
                return false;
            }
            return data[0] == toRep(val);
        }
    };
} // namespace detail
#endif // openPMD_HAVE_ADIOS2

class ADIOS2IOHandler : public AbstractIOHandler
{
#if openPMD_HAVE_ADIOS2

    friend class ADIOS2IOHandlerImpl;

private:
    ADIOS2IOHandlerImpl m_impl;

public:
    ~ADIOS2IOHandler() override
    {
        // we must not throw in a destructor
        try
        {
            auto params = internal::defaultParsedFlushParams;
            this->flush(params);
        }
        catch (std::exception const &ex)
        {
            std::cerr << "[~ADIOS2IOHandler] An error occurred: " << ex.what()
                      << std::endl;
        }
        catch (...)
        {
            std::cerr << "[~ADIOS2IOHandler] An error occurred." << std::endl;
        }
    }

#else
public:
#endif

#if openPMD_HAVE_MPI

    ADIOS2IOHandler(
        std::string path,
        Access,
        MPI_Comm,
        json::TracingJSON options,
        std::string engineType,
        std::string specifiedExtension);

#endif

    ADIOS2IOHandler(
        std::string path,
        Access,
        json::TracingJSON options,
        std::string engineType,
        std::string specifiedExtension);

    std::string backendName() const override
    {
        return "ADIOS2";
    }

    std::future<void> flush(internal::ParsedFlushParams &) override;
}; // ADIOS2IOHandler
} // namespace openPMD
