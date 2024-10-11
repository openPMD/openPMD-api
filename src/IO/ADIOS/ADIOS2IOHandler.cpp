/* Copyright 2017-2021 Franz Poeschel, Fabian Koller and Axel Huebl
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

#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"
#include "openPMD/IO/ADIOS/ADIOS2File.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/ADIOS/ADIOS2Auxiliary.hpp"
#include "openPMD/IO/ADIOS/ADIOS2FilePosition.hpp"
#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/auxiliary/Mpi.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"

#include <algorithm>
#include <cctype> // std::tolower
#include <iostream>
#include <iterator>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace openPMD
{
#if openPMD_USE_VERIFY
#define VERIFY(CONDITION, TEXT)                                                \
    {                                                                          \
        if (!(CONDITION))                                                      \
            throw std::runtime_error((TEXT));                                  \
    }
#else
#define VERIFY(CONDITION, TEXT)                                                \
    do                                                                         \
    {                                                                          \
        (void)sizeof(CONDITION);                                               \
    } while (0);
#endif

#define VERIFY_ALWAYS(CONDITION, TEXT)                                         \
    {                                                                          \
        if (!(CONDITION))                                                      \
            throw std::runtime_error((TEXT));                                  \
    }

#if openPMD_HAVE_ADIOS2

std::optional<size_t> joinedDimension(adios2::Dims const &dims)
{
    for (size_t i = 0; i < dims.size(); ++i)
    {
        if (dims[i] == adios2::JoinedDim)
        {
            return i;
        }
    }
    return std::nullopt;
}

#if openPMD_HAVE_MPI

ADIOS2IOHandlerImpl::ADIOS2IOHandlerImpl(
    AbstractIOHandler *handler,
    MPI_Comm communicator,
    json::TracingJSON cfg,
    std::string engineType,
    std::string specifiedExtension)
    : AbstractIOHandlerImplCommon(handler)
    , m_ADIOS{communicator}
    , m_communicator{communicator}
    , m_engineType(std::move(engineType))
    , m_userSpecifiedExtension{std::move(specifiedExtension)}
{
    init(
        std::move(cfg),
        /* callbackWriteAttributesFromRank = */
        [communicator, this](nlohmann::json const &attribute_writing_ranks) {
            int rank = 0;
            MPI_Comm_rank(communicator, &rank);
            auto throw_error = []() {
                throw error::BackendConfigSchema(
                    {"adios2", "attribute_writing_ranks"},
                    "Type must be either an integer or an array of integers.");
            };
            if (attribute_writing_ranks.is_array())
            {
                m_writeAttributesFromThisRank = false;
                for (auto const &val : attribute_writing_ranks)
                {
                    if (!val.is_number())
                    {
                        throw_error();
                    }
                    if (val.get<int>() == rank)
                    {
                        m_writeAttributesFromThisRank = true;
                        break;
                    }
                }
            }
            else if (attribute_writing_ranks.is_number())
            {
                m_writeAttributesFromThisRank =
                    attribute_writing_ranks.get<int>() == rank;
            }
            else
            {
                throw_error();
            }
        });
}

#endif // openPMD_HAVE_MPI

ADIOS2IOHandlerImpl::ADIOS2IOHandlerImpl(
    AbstractIOHandler *handler,
    json::TracingJSON cfg,
    std::string engineType,
    std::string specifiedExtension)
    : AbstractIOHandlerImplCommon(handler)
    , m_ADIOS{}
    , m_engineType(std::move(engineType))
    , m_userSpecifiedExtension(std::move(specifiedExtension))
{
    init(std::move(cfg), [](auto const &...) {});
}

ADIOS2IOHandlerImpl::~ADIOS2IOHandlerImpl()
{
    /*
     * m_fileData is an unordered_map indexed by pointer addresses
     * to the fileState member of InvalidatableFile.
     * This means that destruction order is nondeterministic.
     * Let's determinize it (necessary if computing in parallel).
     */
    using file_t = std::unique_ptr<detail::ADIOS2File>;
    std::vector<file_t> sorted;
    sorted.reserve(m_fileData.size());
    for (auto &pair : m_fileData)
    {
        sorted.push_back(std::move(pair.second));
    }
    m_fileData.clear();
    /*
     * Technically, std::sort() is sufficient here, since file names are unique.
     * Use std::stable_sort() for two reasons:
     * 1) On some systems (clang 13.0.1, libc++ 13.0.1), std::sort() leads to
     *    weird inconsistent segfaults here.
     * 2) Robustness against future changes. stable_sort() might become needed
     *    in future, and debugging this can be hard.
     * 3) It does not really matter, this is just the destructor, so we can take
     *    the extra time.
     */
    std::stable_sort(
        sorted.begin(), sorted.end(), [](auto const &left, auto const &right) {
            return left->m_file <= right->m_file;
        });
    // run the destructors
    for (auto &file : sorted)
    {
        // std::unique_ptr interface
        file.reset();
    }
}

template <typename Callback>
void ADIOS2IOHandlerImpl::init(
    json::TracingJSON cfg, Callback &&callbackWriteAttributesFromRank)
{
    if (auto unsupported_engine_cfg =
            auxiliary::getEnvString("OPENPMD_ADIOS2_PRETEND_ENGINE", "");
        !unsupported_engine_cfg.empty())
    {
        auxiliary::lowerCase(unsupported_engine_cfg);
        pretendEngine(std::move(unsupported_engine_cfg));
    }
    // allow overriding through environment variable
    realEngineType() =
        auxiliary::getEnvString("OPENPMD_ADIOS2_ENGINE", m_engineType);
    auxiliary::lowerCase(realEngineType());

    // environment-variable based configuration
    if (int groupTableViaEnv =
            auxiliary::getEnvNum("OPENPMD2_ADIOS2_USE_GROUP_TABLE", -1);
        groupTableViaEnv != -1)
    {
        m_useGroupTable =
            groupTableViaEnv == 0 ? UseGroupTable::No : UseGroupTable::Yes;
    }

    if (cfg.json().contains("adios2"))
    {
        m_config = cfg["adios2"];

        if (m_config.json().contains("use_group_table"))
        {
            m_useGroupTable = m_config["use_group_table"].json().get<bool>()
                ? UseGroupTable::Yes
                : UseGroupTable::No;
        }

        if (m_config.json().contains("use_span_based_put"))
        {
            m_useSpanBasedPutByDefault =
                m_config["use_span_based_put"].json().get<bool>() ? UseSpan::Yes
                                                                  : UseSpan::No;
        }

        if (m_config.json().contains("modifiable_attributes"))
        {
            m_modifiableAttributes =
                m_config["modifiable_attributes"].json().get<bool>()
                ? ModifiableAttributes::Yes
                : ModifiableAttributes::No;
        }

        if (m_config.json().contains("attribute_writing_ranks"))
        {
            callbackWriteAttributesFromRank(
                m_config["attribute_writing_ranks"].json());
        }

        auto engineConfig = config(adios_defaults::str_engine);
        if (!engineConfig.json().is_null())
        {
            auto engineTypeConfig =
                config(adios_defaults::str_type, engineConfig).json();
            if (!engineTypeConfig.is_null())
            {
                // convert to string
                auto maybeEngine =
                    json::asLowerCaseStringDynamic(engineTypeConfig);
                if (maybeEngine.has_value())
                {
                    // override engine type by JSON/TOML configuration
                    realEngineType() = std::move(maybeEngine.value());
                }
                else
                {
                    throw error::BackendConfigSchema(
                        {"adios2", "engine", "type"},
                        "Must be convertible to string type.");
                }
            }

            if (engineConfig.json().contains(
                    adios_defaults::str_treat_unsupported_engine_like))
            {
                auto maybeEngine = json::asLowerCaseStringDynamic(
                    engineConfig
                        [adios_defaults::str_treat_unsupported_engine_like]
                            .json());
                if (!maybeEngine.has_value())
                {
                    throw error::BackendConfigSchema(
                        {"adios2",
                         adios_defaults::str_engine,
                         adios_defaults::str_treat_unsupported_engine_like},
                        "Must be convertible to string type.");
                }
                pretendEngine(std::move(*maybeEngine));
            }
        }
        auto operators = getOperators();
        if (operators)
        {
            defaultOperators = std::move(operators.value());
        }
    }
#if !openPMD_HAS_ADIOS_2_9
    if (m_modifiableAttributes == ModifiableAttributes::Yes)
    {
        throw error::OperationUnsupportedInBackend(
            m_handler->backendName(),
            "Modifiable attributes require ADIOS2 >= v2.9.");
    }
    if (m_useGroupTable.has_value() &&
        m_useGroupTable.value() == UseGroupTable::Yes)
    {
        throw error::OperationUnsupportedInBackend(
            m_handler->backendName(),
            "ADIOS2 group table feature requires ADIOS2 >= v2.9.");
    }
#endif
}

std::optional<std::vector<ADIOS2IOHandlerImpl::ParameterizedOperator>>
ADIOS2IOHandlerImpl::getOperators(json::TracingJSON cfg)
{
    using ret_t = std::optional<std::vector<ParameterizedOperator>>;
    std::vector<ParameterizedOperator> res;
    if (!cfg.json().contains("dataset"))
    {
        return ret_t();
    }
    auto datasetConfig = cfg["dataset"];
    if (!datasetConfig.json().contains("operators"))
    {
        return ret_t();
    }
    auto _operators = datasetConfig["operators"];
    nlohmann::json const &operators = _operators.json();
    for (auto operatorIterator = operators.begin();
         operatorIterator != operators.end();
         ++operatorIterator)
    {
        nlohmann::json const &op = operatorIterator.value();
        std::string const &type = op["type"];
        adios2::Params adiosParams;
        if (op.contains("parameters"))
        {
            nlohmann::json const &params = op["parameters"];
            for (auto paramIterator = params.begin();
                 paramIterator != params.end();
                 ++paramIterator)
            {
                auto maybeString = json::asStringDynamic(paramIterator.value());
                if (maybeString.has_value())
                {
                    adiosParams[paramIterator.key()] =
                        std::move(maybeString.value());
                }
                else
                {
                    throw error::BackendConfigSchema(
                        {"adios2", "dataset", "operators", paramIterator.key()},
                        "Must be convertible to string type.");
                }
            }
        }
        std::optional<adios2::Operator> adiosOperator =
            getCompressionOperator(type);
        if (adiosOperator)
        {
            res.emplace_back(ParameterizedOperator{
                adiosOperator.value(), std::move(adiosParams)});
        }
    }
    _operators.declareFullyRead();
    return std::make_optional(std::move(res));
}

std::optional<std::vector<ADIOS2IOHandlerImpl::ParameterizedOperator>>
ADIOS2IOHandlerImpl::getOperators()
{
    return getOperators(m_config);
}

using AcceptedEndingsForEngine = std::map<std::string, std::string>;

std::string ADIOS2IOHandlerImpl::fileSuffix(bool verbose) const
{
    // SST engine adds its suffix unconditionally
    // so we don't add it
#if openPMD_HAVE_ADIOS2_BP5 && openPMD_HAS_ADIOS_2_9
    constexpr char const *const default_file_ending = ".bp5";
#else
    constexpr char const *const default_file_ending = ".bp4";
#endif

    if (m_realEngineType.has_value())
    {
        // unknown engine type, use whatever ending the user specified
        return m_userSpecifiedExtension;
    }

    static std::map<std::string, AcceptedEndingsForEngine> const endings{
        {"sst", {{"", ""}, {".sst", ""}, {".%E", ""}}},
        {"staging", {{"", ""}, {".sst", ""}, {".%E", ""}}},
        {"filestream",
         {{".bp", ".bp"},
          {".bp4", ".bp4"},
          {".bp5", ".bp5"},
          {".%E", default_file_ending}}},
        {"bp4", {{".bp4", ".bp4"}, {".bp", ".bp"}, {".%E", ".bp4"}}},
        {"bp5", {{".bp5", ".bp5"}, {".bp", ".bp"}, {".%E", ".bp5"}}},
        {"bp3", {{".bp", ".bp"}, {".%E", ".bp"}}},
        {"file",
         {{".bp", ".bp"},
          {".bp4", ".bp4"},
          {".bp5", ".bp5"},
          {".%E", default_file_ending}}},
        {"hdf5", {{".h5", ".h5"}, {".%E", ".h5"}}},
        {"nullcore",
         {{".nullcore", ".nullcore"}, {".bp", ".bp"}, {".%E", ".nullcore"}}},
        {"ssc", {{".ssc", ".ssc"}, {".%E", ".ssc"}}}};

    if (auto engine = endings.find(m_engineType); engine != endings.end())
    {
        auto const &acceptedEndings = engine->second;
        if (auto ending = acceptedEndings.find(m_userSpecifiedExtension);
            ending != acceptedEndings.end())
        {
            if (verbose &&
                (m_engineType == "file" || m_engineType == "filestream") &&
                (m_userSpecifiedExtension == ".bp3" ||
                 m_userSpecifiedExtension == ".bp4" ||
                 m_userSpecifiedExtension == ".bp5"))
            {
                std::cerr
                    << "[ADIOS2] Explicit ending '" << m_userSpecifiedExtension
                    << "' was specified in combination with generic file "
                       "engine '"
                    << m_engineType
                    << "'. ADIOS2 will pick a default file ending "
                       "independent of specified suffix. (E.g. 'simData.bp5' "
                       "might actually be written as a BP4 dataset.)"
                    << std::endl;
            }
            return ending->second;
        }
        else if (m_userSpecifiedExtension.empty())
        {
            std::cerr << "[ADIOS2] No file ending specified. Will not add one."
                      << std::endl;
            if (verbose && m_engineType == "bp3")
            {
                std::cerr
                    << "Note that the ADIOS2 BP3 engine will add its "
                       "ending '.bp' if not specified (e.g. 'simData.bp3' "
                       "will appear on disk as 'simData.bp3.bp')."
                    << std::endl;
            }
            return "";
        }
        else
        {
            if (verbose)
            {
                std::cerr << "[ADIOS2] Specified ending '"
                          << m_userSpecifiedExtension
                          << "' does not match the selected engine '"
                          << m_engineType
                          << "'. Will use the specified ending anyway."
                          << std::endl;
                if (m_engineType == "bp3")
                {
                    std::cerr
                        << "Note that the ADIOS2 BP3 engine will add its "
                           "ending '.bp' if not specified (e.g. 'simData.bp3' "
                           "will appear on disk as 'simData.bp3.bp')."
                        << std::endl;
                }
            }
            return m_userSpecifiedExtension;
        }
    }
    else
    {
        throw error::WrongAPIUsage(
            "[ADIOS2] Specified engine '" + m_engineType +
            "' is not supported by ADIOS2 backend.");
    }
}

using FlushTarget = adios_defs::FlushTarget;

static FlushTarget &
overrideFlushTarget(FlushTarget &inplace, FlushTarget new_val)
{
    auto allowsOverride = [](FlushTarget ft) {
        switch (ft)
        {
        case FlushTarget::Buffer:
        case FlushTarget::Disk:
        case FlushTarget::NewStep:
            return true;
        case FlushTarget::Buffer_Override:
        case FlushTarget::Disk_Override:
        case FlushTarget::NewStep_Override:
            return false;
        }
        return true;
    };

    if (allowsOverride(inplace))
    {
        inplace = new_val;
    }
    else
    {
        if (!allowsOverride(new_val))
        {
            inplace = new_val;
        }
        // else { keep the old value, no-op }
    }
    return inplace;
}

std::future<void>
ADIOS2IOHandlerImpl::flush(internal::ParsedFlushParams &flushParams)
{
    auto res = AbstractIOHandlerImpl::flush();

    detail::ADIOS2File::ADIOS2FlushParams adios2FlushParams{
        flushParams.flushLevel, m_flushTarget};
    if (flushParams.backendConfig.json().contains("adios2"))
    {
        auto adios2Config = flushParams.backendConfig["adios2"];
        if (adios2Config.json().contains("engine"))
        {
            auto engineConfig = adios2Config["engine"];
            if (engineConfig.json().contains(adios_defaults::str_flushtarget))
            {
                auto target = json::asLowerCaseStringDynamic(
                    engineConfig[adios_defaults::str_flushtarget].json());
                if (!target.has_value())
                {
                    throw error::BackendConfigSchema(
                        {"adios2", "engine", adios_defaults::str_flushtarget},
                        "Flush target must be either 'disk' or 'buffer', but "
                        "was non-literal type.");
                }
                overrideFlushTarget(
                    adios2FlushParams.flushTarget,
                    adios_defs::flushTargetFromString(target.value()));
            }
        }

        if (auto shadow = adios2Config.invertShadow(); shadow.size() > 0)
        {
            switch (adios2Config.originallySpecifiedAs)
            {
            case json::SupportedLanguages::JSON:
                std::cerr << "Warning: parts of the backend configuration for "
                             "ADIOS2 remain unused:\n"
                          << shadow << std::endl;
                break;
            case json::SupportedLanguages::TOML: {
                auto asToml = json::jsonToToml(shadow);
                std::cerr << "Warning: parts of the backend configuration for "
                             "ADIOS2 remain unused:\n"
                          << json::format_toml(asToml) << std::endl;
                break;
            }
            }
        }
    }

    for (auto const &file : m_dirty)
    {
        auto file_data = m_fileData.find(file);
        if (file_data == m_fileData.end())
        {
            throw error::Internal(
                "[ADIOS2 backend] No associated data found for file'" + *file +
                "'.");
        }
        file_data->second->flush(
            adios2FlushParams, /* writeLatePuts = */ false);
    }
    m_dirty.clear();
    return res;
}

/*
 * If the iteration encoding is variableBased, we default to using a group
 * table, since it is the only reliable way to recover currently active
 * groups.
 * If group-based encoding is used without group table, then
 * READ_LINEAR is forbidden as it will be unreliable in reporting
 * currently available data.
 * Use AbstractIOHandler::m_encoding for implementing this logic.
 */

static constexpr char const *warningADIOS2NoGroupbasedEncoding = &R"(
[Warning] Use of group-based encoding in ADIOS2 is discouraged as it can lead
to drastic performance issues, no matter if I/O steps are used or not.

* If not using I/O steps: A crash will corrupt all data since there is only
  one atomic logical write operation upon closing the file.
  Memory performance can be pathological depending on the setup.
* If using I/O steps: Each step will add new variables and attributes instead
  of reusing those from earlier steps. ADIOS2 is not optimized for this and
  especially the BP5 engine will show a quadratic increase in metadata size
  as the number of steps increase.
We advise you to pick either file-based encoding or variable-based encoding
(variable-based encoding is not yet feature-complete in the openPMD-api).
For more details, refer to
https://openpmd-api.readthedocs.io/en/latest/usage/concepts.html#iteration-and-series)"
                                                                     [1];

void ADIOS2IOHandlerImpl::createFile(
    Writable *writable, Parameter<Operation::CREATE_FILE> const &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[ADIOS2] Creating a file in read-only mode is not possible.");

    if (!writable->written)
    {
        std::string name = parameters.name + fileSuffix();

        auto res_pair = getPossiblyExisting(name);
        InvalidatableFile shared_name = InvalidatableFile(name);
        VERIFY_ALWAYS(
            !(m_handler->m_backendAccess == Access::READ_WRITE &&
              (!std::get<PE_NewlyCreated>(res_pair) ||
               auxiliary::file_exists(
                   fullPath(std::get<PE_InvalidatableFile>(res_pair))))),
            "[ADIOS2] Can only overwrite existing file in CREATE mode.");

        if (!std::get<PE_NewlyCreated>(res_pair))
        {
            auto file = std::get<PE_InvalidatableFile>(res_pair);
            m_dirty.erase(file);
            dropFileData(file);
            file.invalidate();
        }

        std::string const dir(m_handler->directory);
        if (!auxiliary::directory_exists(dir))
        {
            auto success = auxiliary::create_directories(dir);
            VERIFY(success, "[ADIOS2] Could not create directory.");
        }

        associateWithFile(writable, shared_name);
        this->m_dirty.emplace(shared_name);

        writable->written = true;
        writable->abstractFilePosition = std::make_shared<ADIOS2FilePosition>();
        // enforce opening the file
        // lazy opening is deathly in parallel situations
        auto &fileData =
            getFileData(shared_name, IfFileNotOpen::OpenImplicitly);

        if (!printedWarningsAlready.noGroupBased &&
            m_writeAttributesFromThisRank &&
            m_handler->m_encoding == IterationEncoding::groupBased)
        {
            // For a peaceful phase-out of group-based encoding in ADIOS2,
            // print this warning only in the new layout (with group table)
            if (m_useGroupTable.value_or(UseGroupTable::No) ==
                UseGroupTable::Yes)
            {
                std::cerr << warningADIOS2NoGroupbasedEncoding << std::endl;
                printedWarningsAlready.noGroupBased = true;
            }
            fileData.m_IO.DefineAttribute(
                adios_defaults::str_groupBasedWarning,
                std::string("Consider using file-based or variable-based "
                            "encoding instead in ADIOS2."));
        }
    }
}

void ADIOS2IOHandlerImpl::checkFile(
    Writable *, Parameter<Operation::CHECK_FILE> &parameters)
{
    std::string name =
        fullPath(parameters.name + fileSuffix(/* verbose = */ false));

    using FileExists = Parameter<Operation::CHECK_FILE>::FileExists;
    *parameters.fileExists = checkFile(name) ? FileExists::Yes : FileExists::No;
}

bool ADIOS2IOHandlerImpl::checkFile(std::string fullFilePath) const
{
    if (realEngineType() == "bp3")
    {
        if (!auxiliary::ends_with(fullFilePath, ".bp"))
        {
            /*
             * BP3 will add this ending if not specified
             */
            fullFilePath += ".bp";
        }
    }
    else if (realEngineType() == "sst")
    {
        /*
         * SST will add this ending indiscriminately
         */
        fullFilePath += ".sst";
    }
    bool fileExists = auxiliary::directory_exists(fullFilePath) ||
        auxiliary::file_exists(fullFilePath);

#if openPMD_HAVE_MPI
    if (m_communicator.has_value())
    {
        bool fileExistsRes = false;
        int status = MPI_Allreduce(
            &fileExists,
            &fileExistsRes,
            1,
            MPI_C_BOOL,
            MPI_LOR, // logical or
            m_communicator.value());
        if (status != 0)
        {
            throw std::runtime_error("MPI Reduction failed!");
        }
        fileExists = fileExistsRes;
    }
#endif

    return fileExists;
}

void ADIOS2IOHandlerImpl::createPath(
    Writable *writable, const Parameter<Operation::CREATE_PATH> &parameters)
{
    std::string path;
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ true);

    /* Sanitize path */
    if (!auxiliary::starts_with(parameters.path, '/'))
    {
        path = filePositionToString(setAndGetFilePosition(writable)) + "/" +
            auxiliary::removeSlashes(parameters.path);
    }
    else
    {
        path = "/" + auxiliary::removeSlashes(parameters.path);
    }

    /* ADIOS has no concept for explicitly creating paths.
     * They are implicitly created with the paths of variables/attributes. */

    writable->written = true;
    writable->abstractFilePosition =
        std::make_shared<ADIOS2FilePosition>(path, GroupOrDataset::GROUP);

    switch (useGroupTable())
    {
    case UseGroupTable::No:
        break;
    case UseGroupTable::Yes:
        getFileData(file, IfFileNotOpen::ThrowError).markActive(writable);
        break;
    }
}

void ADIOS2IOHandlerImpl::createDataset(
    Writable *writable, const Parameter<Operation::CREATE_DATASET> &parameters)
{
    if (access::readOnly(m_handler->m_backendAccess))
    {
        throw std::runtime_error(
            "[ADIOS2] Creating a dataset in a file opened as read "
            "only is not possible.");
    }
#if !openPMD_HAS_ADIOS_2_9
    if (parameters.joinedDimension.has_value())
    {
        error::throwOperationUnsupportedInBackend(
            "ADIOS2", "Joined Arrays require ADIOS2 >= v2.9");
    }
#endif
    if (!writable->written)
    {
        /* Sanitize name */
        std::string name = auxiliary::removeSlashes(parameters.name);

        auto const file =
            refreshFileFromParent(writable, /* preferParentFile = */ true);
        writable->abstractFilePosition.reset();
        auto filePos = setAndGetFilePosition(writable, name);
        filePos->gd = GroupOrDataset::DATASET;
        auto const varName = nameOfVariable(writable);

        std::vector<ParameterizedOperator> operators;
        json::TracingJSON options =
            json::parseOptions(parameters.options, /* considerFiles = */ false);
        if (options.json().contains("adios2"))
        {
            json::TracingJSON datasetConfig(options["adios2"]);
            auto datasetOperators = getOperators(datasetConfig);

            operators = datasetOperators ? std::move(datasetOperators.value())
                                         : defaultOperators;
        }
        else
        {
            operators = defaultOperators;
        }
        parameters.warnUnusedParameters(
            options,
            "adios2",
            "Warning: parts of the backend configuration for ADIOS2 dataset '" +
                varName + "' remain unused:\n");

        // cast from openPMD::Extent to adios2::Dims
        adios2::Dims shape(parameters.extent.begin(), parameters.extent.end());
        if (auto jd = parameters.joinedDimension; jd.has_value())
        {
            shape[jd.value()] = adios2::JoinedDim;
        }

        auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);

#define HAS_BP5_BLOSC2_BUG                                                     \
    (ADIOS2_VERSION_MAJOR * 100 + ADIOS2_VERSION_MINOR == 209 &&               \
     ADIOS2_VERSION_PATCH <= 1)
#if HAS_BP5_BLOSC2_BUG
        std::string engineType = fileData.getEngine().Type();
        std::transform(
            engineType.begin(),
            engineType.end(),
            engineType.begin(),
            [](unsigned char c) { return std::tolower(c); });
        if (!printedWarningsAlready.blosc2bp5 && engineType == "bp5writer")
        {
            for (auto const &op : operators)
            {
                std::string operatorType = op.op.Type();
                std::transform(
                    operatorType.begin(),
                    operatorType.end(),
                    operatorType.begin(),
                    [](unsigned char c) { return std::tolower(c); });
                if (operatorType == "blosc")
                {
                    std::cerr << &R"(
[Warning] Use BP5+Blosc with care in ADIOS2 v2.9.0 and v2.9.1.
Unreadable data might be created, to mitigate either deactivate Blosc or use BP4+Blosc.
For further details see
https://github.com/ornladios/ADIOS2/issues/3504.
                    )"[1] << std::endl;
                    printedWarningsAlready.blosc2bp5 = true;
                }
            }
        }
#endif
#undef HAS_BP5_BLOSC2_BUG

        switchAdios2VariableType<detail::VariableDefiner>(
            parameters.dtype, fileData.m_IO, varName, operators, shape);
        fileData.invalidateVariablesMap();
        writable->written = true;
        m_dirty.emplace(file);
    }
}

namespace detail
{
    struct DatasetExtender
    {
        template <typename T, typename... Args>
        static void call(
            adios2::IO &IO, std::string const &variable, Extent const &newShape)
        {
            auto var = IO.InquireVariable<T>(variable);
            if (!var)
            {
                throw std::runtime_error(
                    "[ADIOS2] Unable to retrieve variable for resizing: '" +
                    variable + "'.");
            }
            adios2::Dims dims;
            dims.reserve(newShape.size());
            for (auto ext : newShape)
            {
                dims.push_back(ext);
            }
            var.SetShape(dims);
        }

        static constexpr char const *errorMsg = "ADIOS2: extendDataset()";
    };
} // namespace detail

void ADIOS2IOHandlerImpl::extendDataset(
    Writable *writable, const Parameter<Operation::EXTEND_DATASET> &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[ADIOS2] Cannot extend datasets in read-only mode.");
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    std::string name = nameOfVariable(writable);
    auto &filedata = getFileData(file, IfFileNotOpen::ThrowError);
    Datatype dt = detail::fromADIOS2Type(filedata.m_IO.VariableType(name));
    switchAdios2VariableType<detail::DatasetExtender>(
        dt, filedata.m_IO, name, parameters.extent);
}

void ADIOS2IOHandlerImpl::openFile(
    Writable *writable, Parameter<Operation::OPEN_FILE> &parameters)
{
    if (!auxiliary::directory_exists(m_handler->directory))
    {
        throw error::ReadError(
            error::AffectedObject::File,
            error::Reason::Inaccessible,
            "ADIOS2",
            "Supplied directory is not valid: " + m_handler->directory);
    }

    std::string name = parameters.name + fileSuffix();

    auto file = std::get<PE_InvalidatableFile>(getPossiblyExisting(name));

    associateWithFile(writable, file);

    writable->written = true;
    writable->abstractFilePosition = std::make_shared<ADIOS2FilePosition>();

    // enforce opening the file
    // lazy opening is deathly in parallel situations
    auto &fileData = getFileData(file, IfFileNotOpen::OpenImplicitly);
    *parameters.out_parsePreference = fileData.parsePreference;
    m_dirty.emplace(std::move(file));
}

void ADIOS2IOHandlerImpl::closeFile(
    Writable *writable, Parameter<Operation::CLOSE_FILE> const &)
{
    auto fileIterator = m_files.find(writable);
    if (fileIterator != m_files.end())
    {
        // do not invalidate the file
        // it still exists, it is just not open
        auto it = m_fileData.find(fileIterator->second);
        if (it != m_fileData.end())
        {
            /*
             * No need to finalize unconditionally, destructor will take care
             * of it.
             */
            it->second->flush(
                FlushLevel::UserFlush,
                [](detail::ADIOS2File &ba, adios2::Engine &) { ba.finalize(); },
                /* writeLatePuts = */ true,
                /* flushUnconditionally = */ false);
            m_fileData.erase(it);
        }
        m_dirty.erase(fileIterator->second);
        m_files.erase(fileIterator);
    }
}

void ADIOS2IOHandlerImpl::openPath(
    Writable *writable, const Parameter<Operation::OPEN_PATH> &parameters)
{
    /* Sanitize path */
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ true);
    std::string prefix =
        filePositionToString(setAndGetFilePosition(writable->parent));
    std::string suffix = auxiliary::removeSlashes(parameters.path);
    std::string infix =
        suffix.empty() || auxiliary::ends_with(prefix, '/') ? "" : "/";

    /* ADIOS has no concept for explicitly creating paths.
     * They are implicitly created with the paths of variables/attributes. */

    writable->abstractFilePosition = std::make_shared<ADIOS2FilePosition>(
        prefix + infix + suffix, GroupOrDataset::GROUP);
    writable->written = true;

    switch (useGroupTable())
    {
    case UseGroupTable::No:
        break;
    case UseGroupTable::Yes:
        getFileData(file, IfFileNotOpen::ThrowError).markActive(writable);
        break;
    }
}

void ADIOS2IOHandlerImpl::openDataset(
    Writable *writable, Parameter<Operation::OPEN_DATASET> &parameters)
{
    auto name = auxiliary::removeSlashes(parameters.name);
    writable->abstractFilePosition.reset();
    auto pos = setAndGetFilePosition(writable, name);
    pos->gd = GroupOrDataset::DATASET;
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ true);
    auto varName = nameOfVariable(writable);
    auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);
    *parameters.dtype =
        detail::fromADIOS2Type(fileData.m_IO.VariableType(varName));
    switchAdios2VariableType<detail::DatasetOpener>(
        *parameters.dtype, this, file, varName, parameters);
    writable->written = true;
}

void ADIOS2IOHandlerImpl::deleteFile(
    Writable *, const Parameter<Operation::DELETE_FILE> &)
{
    throw std::runtime_error("[ADIOS2] Backend does not support deletion.");
}

void ADIOS2IOHandlerImpl::deletePath(
    Writable *, const Parameter<Operation::DELETE_PATH> &)
{
    throw std::runtime_error("[ADIOS2] Backend does not support deletion.");
}

void ADIOS2IOHandlerImpl::deleteDataset(
    Writable *, const Parameter<Operation::DELETE_DATASET> &)
{
    // call filedata.invalidateVariablesMap
    throw std::runtime_error("[ADIOS2] Backend does not support deletion.");
}

void ADIOS2IOHandlerImpl::deleteAttribute(
    Writable *, const Parameter<Operation::DELETE_ATT> &)
{
    // call filedata.invalidateAttributesMap
    throw std::runtime_error("[ADIOS2] Backend does not support deletion.");
}

void ADIOS2IOHandlerImpl::writeDataset(
    Writable *writable, Parameter<Operation::WRITE_DATASET> &parameters)
{
    VERIFY_ALWAYS(
        access::write(m_handler->m_backendAccess),
        "[ADIOS2] Cannot write data in read-only mode.");
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    detail::ADIOS2File &ba = getFileData(file, IfFileNotOpen::ThrowError);
    detail::BufferedPut bp;
    bp.name = nameOfVariable(writable);
    bp.param = std::move(parameters);
    ba.enqueue(std::move(bp));
    m_dirty.emplace(std::move(file));
    writable->written = true; // TODO erst nach dem Schreiben?
}

void ADIOS2IOHandlerImpl::writeAttribute(
    Writable *writable, const Parameter<Operation::WRITE_ATT> &parameters)
{
    if (!m_writeAttributesFromThisRank)
    {
        return;
    }
#if openPMD_HAS_ADIOS_2_9
    switch (useGroupTable())
    {
    case UseGroupTable::No:
        if (parameters.changesOverSteps ==
            Parameter<Operation::WRITE_ATT>::ChangesOverSteps::Yes)
        {
            // cannot do this
            return;
        }

        break;
    case UseGroupTable::Yes: {
        break;
    }
    default:
        throw std::runtime_error("Unreachable!");
    }
#else
    if (parameters.changesOverSteps ==
        Parameter<Operation::WRITE_ATT>::ChangesOverSteps::Yes)
    {
        // cannot do this
        return;
    }
#endif
    switchType<detail::AttributeWriter>(
        parameters.dtype, this, writable, parameters);
}

void ADIOS2IOHandlerImpl::readDataset(
    Writable *writable, Parameter<Operation::READ_DATASET> &parameters)
{
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    detail::ADIOS2File &ba = getFileData(file, IfFileNotOpen::ThrowError);
    detail::BufferedGet bg;
    bg.name = nameOfVariable(writable);
    bg.param = parameters;
    ba.enqueue(std::move(bg));
    m_dirty.emplace(std::move(file));
}

namespace detail
{
    struct GetSpan
    {
        template <typename T, typename... Args>
        static void call(
            ADIOS2IOHandlerImpl *impl,
            Parameter<Operation::GET_BUFFER_VIEW> &params,
            detail::ADIOS2File &ba,
            std::string const &varName)
        {
            auto &IO = ba.m_IO;
            auto &engine = ba.getEngine();
            adios2::Variable<T> variable = impl->verifyDataset<T>(
                params.offset, params.extent, IO, varName);
            adios2::Dims offset(params.offset.begin(), params.offset.end());
            adios2::Dims extent(params.extent.begin(), params.extent.end());
            variable.SetSelection({std::move(offset), std::move(extent)});
            typename adios2::Variable<T>::Span span = engine.Put(variable);
            params.out->backendManagedBuffer = true;
            /*
             * SIC!
             * Do not emplace span.data() yet.
             * Only call span.data() as soon as the user needs the pointer
             * (will always be propagated to the backend with parameters.update
             *  = true).
             * This avoids repeated resizing of ADIOS2 internal buffers if
             * calling multiple spans.
             */
            // params.out->ptr = span.data();
            unsigned nextIndex;
            if (ba.m_updateSpans.empty())
            {
                nextIndex = 0;
            }
            else
            {
                nextIndex = ba.m_updateSpans.rbegin()->first + 1;
            }
            params.out->viewIndex = nextIndex;
            std::unique_ptr<I_UpdateSpan> updateSpan{
                new UpdateSpan<T>{std::move(span)}};
            ba.m_updateSpans.emplace_hint(
                ba.m_updateSpans.end(), nextIndex, std::move(updateSpan));
        }

        static constexpr char const *errorMsg = "ADIOS2: getBufferView()";
    };

    struct HasOperators
    {
        template <typename T>
        static bool call(std::string const &name, adios2::IO &IO)
        {
            adios2::Variable<T> variable = IO.InquireVariable<T>(name);
            if (!variable)
            {
                return false;
            }
            return !variable.Operations().empty();
        }

        static constexpr char const *errorMsg = "ADIOS2: getBufferView()";
    };
} // namespace detail

void ADIOS2IOHandlerImpl::getBufferView(
    Writable *writable, Parameter<Operation::GET_BUFFER_VIEW> &parameters)
{
    // @todo check access mode
    std::string optInEngines[] = {"bp4", "bp5", "file", "filestream"};
    if (std::none_of(
            begin(optInEngines),
            end(optInEngines),
            [this](std::string const &engine) {
                return engine == this->realEngineType();
            }))
    {
        parameters.out->backendManagedBuffer = false;
        return;
    }
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    detail::ADIOS2File &ba = getFileData(file, IfFileNotOpen::ThrowError);

    std::string name = nameOfVariable(writable);
    switch (m_useSpanBasedPutByDefault)
    {
    case UseSpan::No:
        parameters.out->backendManagedBuffer = false;
        return;
    case UseSpan::Auto:
        if (switchAdios2VariableType<detail::HasOperators>(
                parameters.dtype, name, ba.m_IO))
        {
            parameters.out->backendManagedBuffer = false;
            return;
        }
        break;
    case UseSpan::Yes:
        break;
    }

    if (parameters.update)
    {
        detail::I_UpdateSpan &updater =
            *ba.m_updateSpans.at(parameters.out->viewIndex);
        parameters.out->ptr = updater.update();
        parameters.out->backendManagedBuffer = true;
    }
    else
    {
        switchAdios2VariableType<detail::GetSpan>(
            parameters.dtype, this, parameters, ba, name);
    }
}

namespace detail
{
    template <typename T>
    UpdateSpan<T>::UpdateSpan(adios2::detail::Span<T> span_in)
        : span(std::move(span_in))
    {}

    template <typename T>
    void *UpdateSpan<T>::update()
    {
        return span.data();
    }
} // namespace detail

void ADIOS2IOHandlerImpl::readAttribute(
    Writable *writable, Parameter<Operation::READ_ATT> &parameters)
{
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto pos = setAndGetFilePosition(writable);
    detail::ADIOS2File &ba = getFileData(file, IfFileNotOpen::ThrowError);
    auto name = nameOfAttribute(writable, parameters.name);

    auto type = detail::attributeInfo(ba.m_IO, name, /* verbose = */ true);
    if (type == Datatype::UNDEFINED)
    {
        throw error::ReadError(
            error::AffectedObject::Attribute,
            error::Reason::NotFound,
            "ADIOS2",
            name);
    }

    Datatype ret = switchType<detail::AttributeReader>(
        type, *this, ba.m_IO, name, *parameters.resource);
    *parameters.dtype = ret;
}

void ADIOS2IOHandlerImpl::listPaths(
    Writable *writable, Parameter<Operation::LIST_PATHS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[ADIOS2] Internal error: Writable not marked written during path "
        "listing");
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto pos = setAndGetFilePosition(writable);
    std::string myName = filePositionToString(pos);
    if (!auxiliary::ends_with(myName, '/'))
    {
        myName = myName + '/';
    }

    /*
     * since ADIOS does not have a concept of paths, restore them
     * from variables and attributes.
     */
    auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);

    std::unordered_set<std::string> subdirs;
    /*
     * When reading an attribute, we cannot distinguish
     * whether its containing "folder" is a group or a
     * dataset. If we stumble upon a dataset at the current
     * level (which can be distinguished via variables),
     * we put in in the list 'delete_me' to remove them
     * again later on.
     * Note that the method 'listDatasets' does not have
     * this problem since datasets can be restored solely
     * from variables – attributes don't even need to be
     * inspected.
     */
    std::vector<std::string> delete_me;

    switch (useGroupTable())
    {
    case UseGroupTable::No: {
        std::vector<std::string> vars =
            fileData.availableVariablesPrefixed(myName);
        for (auto var : vars)
        {
            auto firstSlash = var.find_first_of('/');
            if (firstSlash != std::string::npos)
            {
                var = var.substr(0, firstSlash);
                subdirs.emplace(std::move(var));
            }
            else
            { // var is a dataset at the current level
                delete_me.push_back(std::move(var));
            }
        }
        std::vector<std::string> attributes =
            fileData.availableAttributesPrefixed(myName);
        for (auto attr : attributes)
        {
            auto firstSlash = attr.find_first_of('/');
            if (firstSlash != std::string::npos)
            {
                attr = attr.substr(0, firstSlash);
                subdirs.emplace(std::move(attr));
            }
        }
        break;
    }
    case UseGroupTable::Yes: {
        {
            auto tablePrefix = adios_defaults::str_activeTablePrefix + myName;
            std::vector attrs =
                fileData.availableAttributesPrefixed(tablePrefix);
            if (fileData.streamStatus ==
                detail::ADIOS2File::StreamStatus::DuringStep)
            {
                auto currentStep = fileData.currentStep();
                for (auto const &attrName : attrs)
                {
                    using table_t = unsigned long long;
                    auto attr = fileData.m_IO.InquireAttribute<table_t>(
                        tablePrefix + attrName);
                    if (!attr)
                    {
                        std::cerr << "[ADIOS2 backend] Unable to inquire group "
                                     "table value for group '"
                                  << myName << attrName
                                  << "', will pretend it does not exist."
                                  << std::endl;
                        continue;
                    }
                    if (attr.Data()[0] != currentStep)
                    {
                        // group wasn't defined in current step
                        continue;
                    }
                    auto firstSlash = attrName.find_first_of('/');
                    if (firstSlash == std::string::npos)
                    {
                        subdirs.emplace(attrName);
                    }
                    // else // should we maybe consider deeper groups too?
                }
            }
            else
            {
                for (auto const &attrName : attrs)
                {
                    auto firstSlash = attrName.find_first_of('/');
                    if (firstSlash == std::string::npos)
                    {
                        subdirs.emplace(attrName);
                    }
                }
            }
        }
        break;
    }
    }

    for (auto &d : delete_me)
    {
        subdirs.erase(d);
    }
    for (auto &path : subdirs)
    {
        parameters.paths->emplace_back(path);
    }
}

void ADIOS2IOHandlerImpl::listDatasets(
    Writable *writable, Parameter<Operation::LIST_DATASETS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[ADIOS2] Internal error: Writable not marked written during path "
        "listing");
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto pos = setAndGetFilePosition(writable);
    // adios2::Engine & engine = getEngine( file );
    std::string myName = filePositionToString(pos);
    if (!auxiliary::ends_with(myName, '/'))
    {
        myName = myName + '/';
    }

    /*
     * since ADIOS does not have a concept of paths, restore them
     * from variables and attributes.
     */

    auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);

    std::unordered_set<std::string> subdirs;
    for (auto var : fileData.availableVariablesPrefixed(myName))
    {
        // if string still contains a slash, variable is a dataset below the
        // current group
        // we only want datasets contained directly within the current group
        // let's ensure that
        auto firstSlash = var.find_first_of('/');
        if (firstSlash == std::string::npos)
        {
            subdirs.emplace(std::move(var));
        }
    }
    for (auto &dataset : subdirs)
    {
        parameters.datasets->emplace_back(dataset);
    }
}

void ADIOS2IOHandlerImpl::listAttributes(
    Writable *writable, Parameter<Operation::LIST_ATTS> &parameters)
{
    VERIFY_ALWAYS(
        writable->written,
        "[ADIOS2] Internal error: Writable not marked "
        "written during attribute writing");
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto pos = setAndGetFilePosition(writable);
    auto attributePrefix = filePositionToString(pos);
    if (attributePrefix == "/")
    {
        attributePrefix = "";
    }
    auto &ba = getFileData(file, IfFileNotOpen::ThrowError);

    std::vector<std::string> attrs =
        ba.availableAttributesPrefixed(attributePrefix);

    for (auto &rawAttr : attrs)
    {
        auto attr = auxiliary::removeSlashes(rawAttr);
        if (attr.find_last_of('/') == std::string::npos)
        {
            parameters.attributes->push_back(std::move(attr));
        }
    }
}

void ADIOS2IOHandlerImpl::advance(
    Writable *writable, Parameter<Operation::ADVANCE> &parameters)
{
    auto file = m_files.at(writable);
    auto &ba = getFileData(file, IfFileNotOpen::ThrowError);
    *parameters.status = ba.advance(parameters.mode);
}

void ADIOS2IOHandlerImpl::closePath(
    Writable *writable, Parameter<Operation::CLOSE_PATH> const &)
{
    VERIFY_ALWAYS(
        writable->written,
        "[ADIOS2] Cannot close a path that has not been written yet.");
    if (access::readOnly(m_handler->m_backendAccess))
    {
        // nothing to do
        return;
    }
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    auto &fileData = getFileData(file, IfFileNotOpen::ThrowError);
    if (!fileData.optimizeAttributesStreaming)
    {
        return;
    }
    auto position = setAndGetFilePosition(writable);
    auto positionString = filePositionToString(position);
    VERIFY(
        !auxiliary::ends_with(positionString, '/'),
        "[ADIOS2] Position string has unexpected format. This is a bug "
        "in the openPMD API.");

    for (auto const &attr :
         fileData.availableAttributesPrefixed(positionString))
    {
        fileData.m_IO.RemoveAttribute(
            std::string(positionString).append("/").append(attr));
    }
}

void ADIOS2IOHandlerImpl::availableChunks(
    Writable *writable, Parameter<Operation::AVAILABLE_CHUNKS> &parameters)
{
    setAndGetFilePosition(writable);
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    detail::ADIOS2File &ba = getFileData(file, IfFileNotOpen::ThrowError);
    std::string varName = nameOfVariable(writable);
    auto engine = ba.getEngine(); // make sure that data are present
    auto datatype = detail::fromADIOS2Type(ba.m_IO.VariableType(varName));
    bool allSteps = ba.m_mode != adios2::Mode::Read &&
        ba.streamStatus == detail::ADIOS2File::StreamStatus::ReadWithoutStream;
    switchAdios2VariableType<detail::RetrieveBlocksInfo>(
        datatype,
        parameters,
        ba.m_IO,
        engine,
        varName,
        /* allSteps = */ allSteps);
}

void ADIOS2IOHandlerImpl::deregister(
    Writable *writable, Parameter<Operation::DEREGISTER> const &)
{
    m_files.erase(writable);
}

void ADIOS2IOHandlerImpl::touch(
    Writable *writable, Parameter<Operation::TOUCH> const &)
{
    auto file = refreshFileFromParent(writable, /* preferParentFile = */ false);
    m_dirty.emplace(std::move(file));
}

adios2::Mode ADIOS2IOHandlerImpl::adios2AccessMode(std::string const &fullPath)
{
    if (m_config.json().contains("engine") &&
        m_config["engine"].json().contains("access_mode"))
    {
        auto const &access_mode_json = m_config.json({"engine", "access_mode"});
        auto maybe_access_mode_string =
            json::asLowerCaseStringDynamic(access_mode_json);
        if (!maybe_access_mode_string.has_value())
        {
            throw error::BackendConfigSchema(
                {"adios2", "engine", "access_mode"}, "Must be of string type.");
        }
        auto access_mode_string = *maybe_access_mode_string;
        using pair_t = std::pair<char const *, adios2::Mode>;
        constexpr std::array<pair_t, 4> modeNames{
            pair_t{"write", adios2::Mode::Write},
            pair_t{"read", adios2::Mode::Read},
            pair_t{"append", adios2::Mode::Append}
#if openPMD_HAS_ADIOS_2_8
            ,
            pair_t{"readrandomaccess", adios2::Mode::ReadRandomAccess}
#endif
        };
        for (auto const &[name, mode] : modeNames)
        {
            if (name == access_mode_string)
            {
                return mode;
            }
        }
        std::stringstream error;
        error << "Unsupported value '" << access_mode_string
              << "'. Must be one of:";
        for (auto const &pair : modeNames)
        {
            error << " '" << pair.first << "'";
        }
        error << '.';
        throw error::BackendConfigSchema(
            {"adios2", "engine", "access_mode"}, error.str());
    }
    switch (m_handler->m_backendAccess)
    {
    case Access::CREATE:
        return adios2::Mode::Write;
#if openPMD_HAS_ADIOS_2_8
    case Access::READ_LINEAR:
        return adios2::Mode::Read;
    case Access::READ_ONLY:
        return adios2::Mode::ReadRandomAccess;
#else
    case Access::READ_LINEAR:
    case Access::READ_ONLY:
        return adios2::Mode::Read;
#endif
    case Access::READ_WRITE:
        if (auxiliary::directory_exists(fullPath) ||
            auxiliary::file_exists(fullPath))
        {
#if openPMD_HAS_ADIOS_2_8
            return adios2::Mode::ReadRandomAccess;
#else
            return adios2::Mode::Read;
#endif
        }
        else
        {
            return adios2::Mode::Write;
        }
    case Access::APPEND:
        return adios2::Mode::Append;
    }
    throw std::runtime_error("Unreachable!");
}

json::TracingJSON ADIOS2IOHandlerImpl::nullvalue = {
    nlohmann::json(), json::SupportedLanguages::JSON};

std::string ADIOS2IOHandlerImpl::filePositionToString(
    std::shared_ptr<ADIOS2FilePosition> filepos)
{
    return filepos->location;
}

std::shared_ptr<ADIOS2FilePosition> ADIOS2IOHandlerImpl::extendFilePosition(
    std::shared_ptr<ADIOS2FilePosition> const &oldPos, std::string s)
{
    auto path = filePositionToString(oldPos);
    if (!auxiliary::ends_with(path, '/') && !auxiliary::starts_with(s, '/'))
    {
        path = path + "/";
    }
    else if (auxiliary::ends_with(path, '/') && auxiliary::starts_with(s, '/'))
    {
        path = auxiliary::replace_last(path, "/", "");
    }
    return std::make_shared<ADIOS2FilePosition>(
        path + std::move(s), oldPos->gd);
}

std::optional<adios2::Operator>
ADIOS2IOHandlerImpl::getCompressionOperator(std::string const &compression)
{
    adios2::Operator res;
    auto it = m_operators.find(compression);
    if (it == m_operators.end())
    {
        try
        {
            res = m_ADIOS.DefineOperator(compression, compression);
        }
        catch (std::invalid_argument const &e)
        {
            std::cerr << "Warning: ADIOS2 backend does not support compression "
                         "method "
                      << compression << ". Continuing without compression."
                      << "\nOriginal error: " << e.what() << std::endl;
            return std::optional<adios2::Operator>();
        }
        catch (std::string const &s)
        {
            std::cerr << "Warning: ADIOS2 backend does not support compression "
                         "method "
                      << compression << ". Continuing without compression."
                      << "\nOriginal error: " << s << std::endl;
            return std::optional<adios2::Operator>();
        }
        m_operators.emplace(compression, res);
    }
    else
    {
        res = it->second;
    }
    return std::make_optional(adios2::Operator(res));
}

std::string ADIOS2IOHandlerImpl::nameOfVariable(Writable *writable)
{
    auto filepos = setAndGetFilePosition(writable);
    return filePositionToString(filepos);
}

std::string
ADIOS2IOHandlerImpl::nameOfAttribute(Writable *writable, std::string attribute)
{
    auto pos = setAndGetFilePosition(writable);
    return filePositionToString(extendFilePosition(
        pos, auxiliary::removeSlashes(std::move(attribute))));
}

GroupOrDataset ADIOS2IOHandlerImpl::groupOrDataset(Writable *writable)
{
    return setAndGetFilePosition(writable)->gd;
}

detail::ADIOS2File &ADIOS2IOHandlerImpl::getFileData(
    InvalidatableFile const &file, IfFileNotOpen flag)
{
    VERIFY_ALWAYS(
        file.valid(),
        "[ADIOS2] Cannot retrieve file data for a file that has "
        "been overwritten or deleted.")
    auto it = m_fileData.find(file);
    if (it == m_fileData.end())
    {
        switch (flag)
        {
        case IfFileNotOpen::OpenImplicitly: {

            auto res = m_fileData.emplace(
                file, std::make_unique<detail::ADIOS2File>(*this, file));
            return *res.first->second;
        }
        case IfFileNotOpen::ThrowError:
            throw std::runtime_error(
                "[ADIOS2] Requested file has not been opened yet: " +
                (file.fileState ? file.fileState->name : "Unknown file name"));
        }
    }
    else
    {
        return *it->second;
    }
}

void ADIOS2IOHandlerImpl::dropFileData(InvalidatableFile const &file)
{
    auto it = m_fileData.find(file);
    if (it != m_fileData.end())
    {
        it->second->drop();
        m_fileData.erase(it);
    }
}

namespace detail
{
    template <typename T>
    Datatype AttributeReader::call(
        ADIOS2IOHandlerImpl &impl,
        adios2::IO &IO,
        std::string name,
        Attribute::resource &resource)
    {
        (void)impl;
        /*
         * If we store an attribute of boolean type, we store an additional
         * attribute prefixed with '__is_boolean__' to indicate this information
         * that would otherwise be lost. Check whether this has been done.
         */
        using rep = AttributeTypes<bool>::rep;

        if constexpr (std::is_same<T, rep>::value)
        {
            auto attr = IO.InquireAttribute<rep>(name);
            if (!attr)
            {
                throw std::runtime_error(
                    "[ADIOS2] Internal error: Failed reading attribute '" +
                    name + "'.");
            }

            std::string metaAttr;
            metaAttr = adios_defaults::str_isBoolean + name;
            /*
             * In verbose mode, attributeInfo will yield a warning if not
             * finding the requested attribute. Since we expect the attribute
             * not to be present in many cases (i.e. when it is actually not
             * a boolean), let's tell attributeInfo to be quiet.
             */
            auto type = attributeInfo(
                IO,
                metaAttr,
                /* verbose = */ false);

            if (type == determineDatatype<rep>())
            {
                auto meta = IO.InquireAttribute<rep>(metaAttr);
                if (meta.Data().size() == 1 && meta.Data()[0] == 1)
                {
                    resource = bool_repr::fromRep(attr.Data()[0]);
                    return determineDatatype<bool>();
                }
            }
            resource = attr.Data()[0];
        }
        else if constexpr (IsUnsupportedComplex_v<T>)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex "
                "attribute types");
        }
        else if constexpr (auxiliary::IsVector_v<T>)
        {
            auto attr = IO.InquireAttribute<typename T::value_type>(name);
            if (!attr)
            {
                throw std::runtime_error(
                    "[ADIOS2] Internal error: Failed reading attribute '" +
                    name + "'.");
            }
            resource = attr.Data();
        }
        else if constexpr (auxiliary::IsArray_v<T>)
        {
            auto attr = IO.InquireAttribute<typename T::value_type>(name);
            if (!attr)
            {
                throw std::runtime_error(
                    "[ADIOS2] Internal error: Failed reading attribute '" +
                    name + "'.");
            }
            auto data = attr.Data();
            T res;
            for (size_t i = 0; i < data.size(); i++)
            {
                res[i] = data[i];
            }
            resource = res;
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            throw std::runtime_error(
                "Observed boolean attribute. ADIOS2 does not have these?");
        }
        else
        {
            auto attr = IO.InquireAttribute<T>(name);
            if (!attr)
            {
                throw std::runtime_error(
                    "[ADIOS2] Internal error: Failed reading attribute '" +
                    name + "'.");
            }
            resource = attr.Data()[0];
        }

        return determineDatatype<T>();
    }

    template <int n, typename... Params>
    Datatype AttributeReader::call(Params &&...)
    {
        throw std::runtime_error(
            "[ADIOS2] Internal error: Unknown datatype while "
            "trying to read an attribute.");
    }

    template <typename T>
    void AttributeWriter::call(
        ADIOS2IOHandlerImpl *impl,
        Writable *writable,
        const Parameter<Operation::WRITE_ATT> &parameters)
    {
        VERIFY_ALWAYS(
            access::write(impl->m_handler->m_backendAccess),
            "[ADIOS2] Cannot write attribute in read-only mode.");
        auto pos = impl->setAndGetFilePosition(writable);
        auto file = impl->refreshFileFromParent(
            writable, /* preferParentFile = */ false);
        auto fullName = impl->nameOfAttribute(writable, parameters.name);
        auto prefix = impl->filePositionToString(pos);

        auto &filedata = impl->getFileData(
            file, ADIOS2IOHandlerImpl::IfFileNotOpen::ThrowError);
        filedata.invalidateAttributesMap();
        adios2::IO IO = filedata.m_IO;
        impl->m_dirty.emplace(std::move(file));

#if openPMD_HAS_ADIOS_2_9
        if (impl->m_modifiableAttributes ==
                ADIOS2IOHandlerImpl::ModifiableAttributes::No &&
            parameters.changesOverSteps ==
                Parameter<Operation::WRITE_ATT>::ChangesOverSteps::No)
#endif // we only support modifiable attrs for ADIOS2 >= 2.9, so no `if`
        {
            std::string t = IO.AttributeType(fullName);
            if (!t.empty()) // an attribute is present <=> it has a type
            {
                // don't overwrite attributes if they are equivalent
                // overwriting is only legal within the same step

                auto attributeModifiable = [&filedata, &fullName]() {
                    auto it = filedata.uncommittedAttributes.find(fullName);
                    return it != filedata.uncommittedAttributes.end();
                };
                if (AttributeTypes<T>::attributeUnchanged(
                        IO, fullName, std::get<T>(parameters.resource)))
                {
                    return;
                }
                else if (attributeModifiable())
                {
                    if (detail::fromADIOS2Type(t) !=
                        basicDatatype(determineDatatype<T>()))
                    {
                        if (impl->m_engineType == "bp5")
                        {
                            throw error::OperationUnsupportedInBackend(
                                "ADIOS2",
                                "Attempting to change datatype of attribute '" +
                                    fullName +
                                    "'. In the BP5 engine, this will lead to "
                                    "corrupted "
                                    "datasets.");
                        }
                        else
                        {
                            std::cerr
                                << "[ADIOS2] Attempting to change datatype "
                                   "of attribute '"
                                << fullName
                                << "'. This invokes undefined behavior. Will "
                                   "proceed."
                                << std::endl;
                        }
                    }
                    IO.RemoveAttribute(fullName);
                }
                else
                {
                    std::cerr
                        << "[Warning][ADIOS2] Cannot modify attribute from "
                           "previous step: "
                        << fullName << std::endl;
                    return;
                }
            }
            else
            {
                filedata.uncommittedAttributes.emplace(fullName);
            }
        }

        auto &value = std::get<T>(parameters.resource);
#if openPMD_HAS_ADIOS_2_9
        bool modifiable = impl->m_modifiableAttributes ==
                ADIOS2IOHandlerImpl::ModifiableAttributes::Yes ||
            parameters.changesOverSteps !=
                Parameter<Operation::WRITE_ATT>::ChangesOverSteps::No;
#else
        bool modifiable = impl->m_modifiableAttributes ==
                ADIOS2IOHandlerImpl::ModifiableAttributes::Yes ||
            parameters.changesOverSteps ==
                Parameter<Operation::WRITE_ATT>::ChangesOverSteps::Yes;
#endif

        auto defineAttribute =
            [&IO, &fullName, &modifiable, &impl](auto const &...args) {
#if openPMD_HAS_ADIOS_2_9
                (void)impl;
                auto attr = IO.DefineAttribute(
                    fullName,
                    args...,
                    /* variableName = */ "",
                    /* separator = */ "/",
                    /* allowModification = */ modifiable);
#else
                /*
                 * Defensive coding, normally this condition should be checked
                 * before getting this far.
                 */
                if (modifiable)
                {
                    throw error::OperationUnsupportedInBackend(
                        impl->m_handler->backendName(),
                        "Modifiable attributes require ADIOS2 >= v2.8.");
                }
                auto attr = IO.DefineAttribute(fullName, args...);
#endif
                if (!attr)
                {
                    throw std::runtime_error(
                        "[ADIOS2] Internal error: Failed defining attribute '" +
                        fullName + "'.");
                }
            };

        /*
         * Some old compilers don't like that this is not used in every
         * instantiation.
         */
        (void)defineAttribute;

        if constexpr (IsUnsupportedComplex_v<T>)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: no support for long double complex "
                "attribute types");
        }
        else if constexpr (auxiliary::IsVector_v<T> || auxiliary::IsArray_v<T>)
        {
            defineAttribute(value.data(), value.size());
        }
        else if constexpr (std::is_same_v<T, bool>)
        {
            IO.DefineAttribute<bool_representation>(
                adios_defaults::str_isBoolean + fullName, 1);
            auto representation = bool_repr::toRep(value);
            defineAttribute(representation);
        }
        else
        {
            defineAttribute(value);
        }
    }

    template <int n, typename... Params>
    void AttributeWriter::call(Params &&...)
    {
        throw std::runtime_error(
            "[ADIOS2] Internal error: Unknown datatype while "
            "trying to write an attribute.");
    }

    template <typename T>
    void DatasetOpener::call(
        ADIOS2IOHandlerImpl *impl,
        InvalidatableFile const &file,
        const std::string &varName,
        Parameter<Operation::OPEN_DATASET> &parameters)
    {
        auto &fileData = impl->getFileData(
            file, ADIOS2IOHandlerImpl::IfFileNotOpen::ThrowError);
        auto &IO = fileData.m_IO;
        adios2::Variable<T> var = IO.InquireVariable<T>(varName);
        if (!var)
        {
            throw std::runtime_error(
                "[ADIOS2] Failed retrieving ADIOS2 Variable with name '" +
                varName + "' from file " + *file + ".");
        }

        // Operators in reading needed e.g. for setting decompression threads
        for (auto const &operation : impl->defaultOperators)
        {
            if (operation.op)
            {
                var.AddOperation(operation.op, operation.params);
            }
        }
        // cast from adios2::Dims to openPMD::Extent
        auto const shape = var.Shape();
        parameters.extent->clear();
        parameters.extent->reserve(shape.size());
        std::copy(
            shape.begin(), shape.end(), std::back_inserter(*parameters.extent));
    }

    template <class>
    inline constexpr bool always_false_v = false;

    template <typename T>
    void VariableDefiner::call(
        adios2::IO &IO,
        std::string const &name,
        std::vector<ADIOS2IOHandlerImpl::ParameterizedOperator> const
            &compressions,
        adios2::Dims const &shape,
        adios2::Dims const &start,
        adios2::Dims const &count,
        bool const constantDims)
    {
        /*
         * Step/Variable-based iteration layout:
         * The variable may already be defined from a previous step,
         * so check if it's already here.
         */
        adios2::Variable<T> var = IO.InquireVariable<T>(name);
        if (!var)
        {
            var = IO.DefineVariable<T>(name, shape, start, count, constantDims);
        }
        else
        {
            var.SetShape(shape);
            if (count.size() > 0)
            {
                var.SetSelection({start, count});
            }
            // don't add compression operators multiple times
            return;
        }

        if (!var)
        {
            throw std::runtime_error(
                "[ADIOS2] Internal error: Could not create Variable '" + name +
                "'.");
        }
        for (auto const &compression : compressions)
        {
            if (compression.op)
            {
                var.AddOperation(compression.op, compression.params);
            }
        }
    }

    template <typename T>
    void RetrieveBlocksInfo::call(
        Parameter<Operation::AVAILABLE_CHUNKS> &params,
        adios2::IO &IO,
        adios2::Engine &engine,
        std::string const &varName,
        bool allSteps)
    {
        auto var = IO.InquireVariable<T>(varName);
        auto &table = *params.chunks;
        auto addBlocksInfo = [&table](auto const &blocksInfo_) {
            for (auto const &info : blocksInfo_)
            {
                Offset offset;
                Extent extent;
                auto size = info.Start.size();
                offset.reserve(size);
                extent.reserve(size);
                for (unsigned i = 0; i < size; ++i)
                {
                    offset.push_back(info.Start[i]);
                    extent.push_back(info.Count[i]);
                }
                table.emplace_back(
                    std::move(offset), std::move(extent), info.WriterID);
            }
        };
        if (allSteps)
        {
            auto allBlocks = var.AllStepsBlocksInfo();
            table.reserve(std::accumulate(
                allBlocks.begin(),
                allBlocks.end(),
                size_t(0),
                [](size_t acc, auto const &block) {
                    return acc + block.size();
                }));
            for (auto const &blocksInfo : allBlocks)
            {
                addBlocksInfo(blocksInfo);
            }
        }
        else
        {
            auto blocksInfo = engine.BlocksInfo<T>(var, engine.CurrentStep());
            table.reserve(blocksInfo.size());
            addBlocksInfo(blocksInfo);
        }
    }

    template <int n, typename... Args>
    void RetrieveBlocksInfo::call(Args &&...)
    {
        // variable has not been found, so we don't fill in any blocks
    }
} // namespace detail

#if openPMD_HAVE_MPI

ADIOS2IOHandler::ADIOS2IOHandler(
    std::string path,
    openPMD::Access at,
    MPI_Comm comm,
    json::TracingJSON options,
    std::string engineType,
    std::string specifiedExtension)
    : AbstractIOHandler(std::move(path), at, comm)
    , m_impl{
          this,
          comm,
          std::move(options),
          std::move(engineType),
          std::move(specifiedExtension)}
{}

#endif

ADIOS2IOHandler::ADIOS2IOHandler(
    std::string path,
    Access at,
    json::TracingJSON options,
    std::string engineType,
    std::string specifiedExtension)
    : AbstractIOHandler(std::move(path), at)
    , m_impl{
          this,
          std::move(options),
          std::move(engineType),
          std::move(specifiedExtension)}
{}

std::future<void>
ADIOS2IOHandler::flush(internal::ParsedFlushParams &flushParams)
{
    return m_impl.flush(flushParams);
}

#else // openPMD_HAVE_ADIOS2

#if openPMD_HAVE_MPI
ADIOS2IOHandler::ADIOS2IOHandler(
    std::string path,
    Access at,
    MPI_Comm comm,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    json::TracingJSON,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::string,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::string)
    : AbstractIOHandler(std::move(path), at, comm)
{}

#endif // openPMD_HAVE_MPI

ADIOS2IOHandler::ADIOS2IOHandler(
    std::string path,
    Access at,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    json::TracingJSON,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::string,
    // NOLINTNEXTLINE(performance-unnecessary-value-param)
    std::string)
    : AbstractIOHandler(std::move(path), at)
{}

std::future<void> ADIOS2IOHandler::flush(internal::ParsedFlushParams &)
{
    return std::future<void>();
}

#endif

} // namespace openPMD
