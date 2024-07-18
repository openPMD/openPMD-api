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

#include "openPMD/IO/ADIOS/ADIOS2File.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/ADIOS/ADIOS2IOHandler.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/auxiliary/Environment.hpp"
#include "openPMD/auxiliary/StringManip.hpp"

#include <stdexcept>

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
namespace openPMD::detail
{
template <typename T>
void DatasetReader::call(
    ADIOS2IOHandlerImpl *impl,
    detail::BufferedGet &bp,
    adios2::IO &IO,
    adios2::Engine &engine,
    std::string const &fileName)
{
    adios2::Variable<T> var =
        impl->verifyDataset<T>(bp.param.offset, bp.param.extent, IO, bp.name);
    if (!var)
    {
        throw std::runtime_error(
            "[ADIOS2] Failed retrieving ADIOS2 Variable with name '" + bp.name +
            "' from file " + fileName + ".");
    }
    auto ptr = std::static_pointer_cast<T>(bp.param.data).get();
    engine.Get(var, ptr);
}

template <class>
inline constexpr bool always_false_v = false;

template <typename T>
void WriteDataset::call(ADIOS2File &ba, detail::BufferedPut &bp)
{
    VERIFY_ALWAYS(
        access::write(ba.m_impl->m_handler->m_backendAccess),
        "[ADIOS2] Cannot write data in read-only mode.");

    std::visit(
        [&](auto &&arg) {
            using ptr_type = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<ptr_type, std::shared_ptr<void const>>)
            {
                auto ptr = static_cast<T const *>(arg.get());

                adios2::Variable<T> var = ba.m_impl->verifyDataset<T>(
                    bp.param.offset, bp.param.extent, ba.m_IO, bp.name);

                ba.getEngine().Put(var, ptr);
            }
            else if constexpr (std::is_same_v<
                                   ptr_type,
                                   UniquePtrWithLambda<void>>)
            {
                BufferedUniquePtrPut bput;
                bput.name = std::move(bp.name);
                bput.offset = std::move(bp.param.offset);
                bput.extent = std::move(bp.param.extent);
                /*
                 * Note: Moving is required here since it's a unique_ptr.
                 * std::forward<>() would theoretically work, but it
                 * requires the type parameter and we don't have that
                 * inside the lambda.
                 * (ptr_type does not work for this case).
                 */
                // clang-format off
                    bput.data = std::move(arg); // NOLINT(bugprone-move-forwarding-reference)
                // clang-format on
                bput.dtype = bp.param.dtype;
                ba.m_uniquePtrPuts.push_back(std::move(bput));
            }
            else
            {
                static_assert(
                    always_false_v<ptr_type>, "Unhandled std::variant branch");
            }
        },
        bp.param.data.m_buffer);
}

template <int n, typename... Params>
void WriteDataset::call(Params &&...)
{
    throw std::runtime_error("[ADIOS2] WRITE_DATASET: Invalid datatype.");
}

void BufferedGet::run(ADIOS2File &ba)
{
    switchAdios2VariableType<detail::DatasetReader>(
        param.dtype, ba.m_impl, *this, ba.m_IO, ba.getEngine(), ba.m_file);
}

void BufferedPut::run(ADIOS2File &ba)
{
    switchAdios2VariableType<detail::WriteDataset>(param.dtype, ba, *this);
}

struct RunUniquePtrPut
{
    template <typename T>
    static void call(BufferedUniquePtrPut &bufferedPut, ADIOS2File &ba)
    {
        auto ptr = static_cast<T const *>(bufferedPut.data.get());
        adios2::Variable<T> var = ba.m_impl->verifyDataset<T>(
            bufferedPut.offset, bufferedPut.extent, ba.m_IO, bufferedPut.name);
        ba.getEngine().Put(var, ptr);
    }

    static constexpr char const *errorMsg = "RunUniquePtrPut";
};

void BufferedUniquePtrPut::run(ADIOS2File &ba)
{
    switchAdios2VariableType<RunUniquePtrPut>(dtype, *this, ba);
}

ADIOS2File::ADIOS2File(ADIOS2IOHandlerImpl &impl, InvalidatableFile file)
    : m_file(impl.fullPath(std::move(file)))
    , m_ADIOS(impl.m_ADIOS)
    , m_impl(&impl)
{
    // Declaring these members in the constructor body to avoid
    // initialization order hazards. Need the IO_ prefix since in some
    // situation there seems to be trouble with number-only IO names
    m_mode = impl.adios2AccessMode(m_file);
    create_IO();
    if (!m_IO)
    {
        throw std::runtime_error(
            "[ADIOS2] Internal error: Failed declaring ADIOS2 IO object "
            "for file " +
            m_file);
    }
    else
    {
        configure_IO();
    }
}

auto ADIOS2File::useGroupTable() const -> UseGroupTable
{
    return m_impl->useGroupTable();
}

void ADIOS2File::create_IO()
{
    m_IOName = std::to_string(m_impl->nameCounter++);
    m_IO = m_impl->m_ADIOS.DeclareIO("IO_" + m_IOName);
}

ADIOS2File::~ADIOS2File()
{
    finalize();
}

void ADIOS2File::finalize()
{
    if (finalized)
    {
        return;
    }
    // if write accessing, ensure that the engine is opened
    // and that all datasets are written
    // (attributes and unique_ptr datasets are written upon closing a step
    // or a file which users might never do)
    bool needToWrite = !m_uniquePtrPuts.empty();
    if ((needToWrite || !m_engine) && writeOnly(m_mode))
    {
        getEngine();
        for (auto &entry : m_uniquePtrPuts)
        {
            entry.run(*this);
        }
    }
    if (m_engine)
    {
        auto &engine = m_engine.value();
        // might have been closed previously
        if (engine)
        {
            if (streamStatus == StreamStatus::DuringStep)
            {
                engine.EndStep();
            }
            engine.Close();
            m_ADIOS.RemoveIO(m_IOName);
        }
    }
    finalized = true;
}

namespace
{
    constexpr char const *alwaysSupportsUpfrontParsing[] = {"bp3", "hdf5"};
    constexpr char const *supportsUpfrontParsingInRandomAccessMode[] = {
        "bp4", "bp5", "file", "filestream"};
    constexpr char const *nonPersistentEngines[] = {
        "sst", "insitumpi", "inline", "staging", "nullcore", "ssc"};

    bool supportedEngine(std::string const &engineType)
    {
        auto is_in_list = [&engineType](auto &list) {
            for (auto const &e : list)
            {
                if (engineType == e)
                {
                    return true;
                }
            }
            return false;
        };
        return is_in_list(alwaysSupportsUpfrontParsing) ||
            is_in_list(supportsUpfrontParsingInRandomAccessMode) ||
            is_in_list(nonPersistentEngines);
    }

    bool supportsUpfrontParsing(Access access, std::string const &engineType)
    {
        for (auto const &e : alwaysSupportsUpfrontParsing)
        {
            if (e == engineType)
            {
                return true;
            }
        }
        if (access != Access::READ_LINEAR)
        {
            for (auto const &e : supportsUpfrontParsingInRandomAccessMode)
            {
                if (e == engineType)
                {
                    return true;
                }
            }
        }
        return false;
    }

    enum class PerstepParsing
    {
        Supported,
        Unsupported,
        Required
    };

    PerstepParsing
    supportsPerstepParsing(Access access, std::string const &engineType)
    {
        // required in all streaming engines
        for (auto const &e : nonPersistentEngines)
        {
            if (engineType == e)
            {
                return PerstepParsing::Required;
            }
        }
        // supported in file engines in READ_LINEAR mode
        if (access != Access::READ_RANDOM_ACCESS)
        {
            return PerstepParsing::Supported;
        }

        return PerstepParsing::Unsupported;
    }

    bool nonpersistentEngine(std::string const &engineType)
    {
        for (auto &e : nonPersistentEngines)
        {
            if (e == engineType)
            {
                return true;
            }
        }
        return false;
    }
} // namespace

size_t ADIOS2File::currentStep()
{
    if (nonpersistentEngine(m_impl->m_engineType))
    {
        return m_currentStep;
    }
    else
    {
        return getEngine().CurrentStep();
    }
}

void ADIOS2File::configure_IO_Read()
{
    bool upfrontParsing = supportsUpfrontParsing(
        m_impl->m_handler->m_backendAccess, m_impl->m_engineType);
    PerstepParsing perstepParsing = supportsPerstepParsing(
        m_impl->m_handler->m_backendAccess, m_impl->m_engineType);

    switch (m_impl->m_handler->m_backendAccess)
    {
    case Access::READ_LINEAR:
        switch (perstepParsing)
        {
        case PerstepParsing::Supported:
        case PerstepParsing::Required:
            // all is fine, we can go forward with READ_LINEAR mode
            /*
             * We don't know yet if per-step parsing will be fine since the
             * engine is not opened yet.
             * In non-persistent (streaming) engines, per-step parsing is
             * always fine and always required.
             */
            streamStatus = nonpersistentEngine(m_impl->m_engineType)
                ? StreamStatus::OutsideOfStep
                : StreamStatus::Undecided;
            parsePreference = ParsePreference::PerStep;
            m_IO.SetParameter("StreamReader", "On");
            break;
        case PerstepParsing::Unsupported:
            throw error::Internal(
                "Internal control flow error: Per-Step parsing cannot be "
                "unsupported when access type is READ_LINEAR");
            break;
        }
        break;
    case Access::READ_ONLY:
    case Access::READ_WRITE:
        /*
         * Prefer up-front parsing, but try to fallback to per-step parsing
         * if possible.
         */
        if (upfrontParsing == nonpersistentEngine(m_impl->m_engineType))
        {
            throw error::Internal(
                "Internal control flow error: With access types "
                "READ_ONLY/READ_WRITE, support for upfront parsing is "
                "equivalent to the chosen engine being file-based.");
        }
        if (upfrontParsing)
        {
            streamStatus = StreamStatus::ReadWithoutStream;
            parsePreference = ParsePreference::UpFront;
        }
        else
        {
            /*
             * Scenario: A step-only workflow was used (i.e. a streaming
             * engine), but Access::READ_ONLY was specified.
             * Fall back to streaming read mode.
             */
            m_mode = adios2::Mode::Read;
            parsePreference = ParsePreference::PerStep;
            streamStatus = StreamStatus::OutsideOfStep;
        }
        break;
    default:
        VERIFY_ALWAYS(
            access::writeOnly(m_impl->m_handler->m_backendAccess),
            "Internal control flow error: Must set parse preference for "
            "any read mode.");
    }
}

void ADIOS2File::configure_IO_Write()
{
    optimizeAttributesStreaming =
        // Also, it should only be done when truly streaming, not
        // when using a disk-based engine that behaves like a
        // streaming engine (otherwise attributes might vanish)
        nonpersistentEngine(m_impl->m_engineType);

    streamStatus = StreamStatus::OutsideOfStep;
}

void ADIOS2File::configure_IO()
{
    // step/variable-based iteration encoding requires use of group tables
    // but the group table feature is available only in ADIOS2 >= v2.9
    // use old layout to support at least one single iteration otherwise
    // these properties are inferred from the opened dataset in read mode
    if (writeOnly(m_mode))
    {

#if openPMD_HAS_ADIOS_2_9
        if (!m_impl->m_useGroupTable.has_value())
        {
            switch (m_impl->m_handler->m_encoding)
            {
            /*
             * For variable-based encoding, this does not matter as it is
             * new and requires >= v2.9 features anyway.
             */
            case IterationEncoding::variableBased:
                m_impl->m_useGroupTable = UseGroupTable::Yes;
                break;
            case IterationEncoding::groupBased:
            case IterationEncoding::fileBased:
                m_impl->m_useGroupTable = UseGroupTable::No;
                break;
            }
        }

        if (m_impl->m_modifiableAttributes ==
            ADIOS2IOHandlerImpl::ModifiableAttributes::Unspecified)
        {
            m_impl->m_modifiableAttributes = m_impl->m_handler->m_encoding ==
                    IterationEncoding::variableBased
                ? ADIOS2IOHandlerImpl::ModifiableAttributes::Yes
                : ADIOS2IOHandlerImpl::ModifiableAttributes::No;
        }
#else
        if (!m_impl->m_useGroupTable.has_value())
        {
            m_impl->m_useGroupTable = UseGroupTable::No;
        }

        m_impl->m_modifiableAttributes =
            ADIOS2IOHandlerImpl::ModifiableAttributes::No;
#endif
    }

    // set engine type
    {
        m_IO.SetEngine(m_impl->realEngineType());
    }

    if (!supportedEngine(m_impl->m_engineType))
    {
        std::stringstream sstream;
        sstream << "User-selected ADIOS2 engine '" << m_impl->m_engineType
                << "' is not recognized by the openPMD-api. Select one of: '";
        bool first_entry = true;
        auto add_entries = [&first_entry, &sstream](auto &list) {
            for (auto const &e : list)
            {
                if (first_entry)
                {
                    sstream << e;
                    first_entry = false;
                }
                else
                {
                    sstream << ", " << e;
                }
            }
        };
        add_entries(alwaysSupportsUpfrontParsing);
        add_entries(supportsUpfrontParsingInRandomAccessMode);
        add_entries(nonPersistentEngines);
        sstream << "'." << std::endl;
        throw error::WrongAPIUsage(sstream.str());
    }

    // set engine parameters
    std::set<std::string> alreadyConfigured;
    bool wasTheFlushTargetSpecifiedViaJSON = false;
    auto engineConfig = m_impl->config(adios_defaults::str_engine);
    if (!engineConfig.json().is_null())
    {
        auto params = m_impl->config(adios_defaults::str_params, engineConfig);
        params.declareFullyRead();
        if (params.json().is_object())
        {
            for (auto it = params.json().begin(); it != params.json().end();
                 it++)
            {
                auto maybeString = json::asStringDynamic(it.value());
                if (maybeString.has_value())
                {
                    m_IO.SetParameter(it.key(), std::move(maybeString.value()));
                }
                else
                {
                    throw error::BackendConfigSchema(
                        {"adios2", "engine", "parameters", it.key()},
                        "Must be convertible to string type.");
                }
                alreadyConfigured.emplace(
                    auxiliary::lowerCase(std::string(it.key())));
            }
        }
        auto _useAdiosSteps =
            m_impl->config(adios_defaults::str_usesteps, engineConfig);
        if (!_useAdiosSteps.json().is_null() && writeOnly(m_mode))
        {
            std::cerr << "[ADIOS2 backend] WARNING: Parameter "
                         "`adios2.engine.usesteps` is deprecated since use "
                         "of steps is now always enabled."
                      << std::endl;
        }

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
            m_impl->m_flushTarget =
                adios_defs::flushTargetFromString(target.value());
            wasTheFlushTargetSpecifiedViaJSON = true;
        }
    }

    auto shadow = m_impl->m_config.invertShadow();
    if (shadow.size() > 0)
    {
        switch (m_impl->m_config.originallySpecifiedAs)
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

    switch (m_impl->m_handler->m_backendAccess)
    {
    case Access::READ_LINEAR:
    case Access::READ_ONLY:
        configure_IO_Read();
        break;
    case Access::READ_WRITE:
        if (readOnly(m_mode))
        {
            configure_IO_Read();
        }
        else
        {
            configure_IO_Write();
        }
        break;
    case Access::APPEND:
    case Access::CREATE:
        configure_IO_Write();
        break;
    }

    auto notYetConfigured = [&alreadyConfigured](std::string const &param) {
        auto it =
            alreadyConfigured.find(auxiliary::lowerCase(std::string(param)));
        return it == alreadyConfigured.end();
    };

    // read parameters from environment
    if (notYetConfigured("CollectiveMetadata"))
    {
        if (1 == auxiliary::getEnvNum("OPENPMD_ADIOS2_HAVE_METADATA_FILE", 1))
        {
            m_IO.SetParameter("CollectiveMetadata", "On");
        }
        else
        {
            m_IO.SetParameter("CollectiveMetadata", "Off");
        }
    }
    if (notYetConfigured("Profile"))
    {
        if (1 == auxiliary::getEnvNum("OPENPMD_ADIOS2_HAVE_PROFILING", 1) &&
            notYetConfigured("Profile"))
        {
            m_IO.SetParameter("Profile", "On");
        }
        else
        {
            m_IO.SetParameter("Profile", "Off");
        }
    }
    if (notYetConfigured("AsyncWrite"))
    {
        if (1 == auxiliary::getEnvNum("OPENPMD_ADIOS2_ASYNC_WRITE", 0) &&
            notYetConfigured("AsyncWrite"))
        {
            m_IO.SetParameter("AsyncWrite", "On");
            if (!wasTheFlushTargetSpecifiedViaJSON)
            {
                m_impl->m_flushTarget = FlushTarget::Buffer;
            }
        }
        else
        {
            m_IO.SetParameter("AsyncWrite", "Off");
        }
    }

#if openPMD_HAVE_MPI
    {
        auto num_substreams =
            auxiliary::getEnvNum("OPENPMD_ADIOS2_NUM_SUBSTREAMS", 0);
        if (notYetConfigured("SubStreams") && 0 != num_substreams)
        {
            m_IO.SetParameter("SubStreams", std::to_string(num_substreams));
        }

        // BP5 parameters
        auto numAgg = auxiliary::getEnvNum("OPENPMD_ADIOS2_BP5_NumAgg", 0);
        auto numSubFiles =
            auxiliary::getEnvNum("OPENPMD_ADIOS2_BP5_NumSubFiles", 0);
        auto AggTypeStr =
            auxiliary::getEnvString("OPENPMD_ADIOS2_BP5_TypeAgg", "");
        auto MaxShmMB = auxiliary::getEnvNum("OPENPMD_ADIOS2_BP5_MaxShmMB", 0);
        auto BufferChunkMB =
            auxiliary::getEnvNum("OPENPMD_ADIOS2_BP5_BufferChunkMB", 0);

        if (notYetConfigured("NumAggregators") && (numAgg > 0))
            m_IO.SetParameter("NumAggregators", std::to_string(numAgg));
        if (notYetConfigured("NumSubFiles") && (numSubFiles > 0))
            m_IO.SetParameter("NumSubFiles", std::to_string(numSubFiles));
        if (notYetConfigured("AggregationType") && (AggTypeStr.size() > 0))
            m_IO.SetParameter("AggregationType", AggTypeStr);
        if (notYetConfigured("BufferChunkSize") && (BufferChunkMB > 0))
            m_IO.SetParameter(
                "BufferChunkSize",
                std::to_string((uint64_t)BufferChunkMB * (uint64_t)1048576));
        if (notYetConfigured("MaxShmSize") && (MaxShmMB > 0))
            m_IO.SetParameter(
                "MaxShmSize",
                std::to_string((uint64_t)MaxShmMB * (uint64_t)1048576));
    }
#endif
    if (notYetConfigured("StatsLevel"))
    {
        /*
         * Switch those off by default since they are expensive to compute
         * and to enable it, set the JSON option "StatsLevel" or the
         * environment variable "OPENPMD_ADIOS2_STATS_LEVEL" be positive.
         * The ADIOS2 default was "1" (on).
         */
        auto stats_level =
            auxiliary::getEnvNum("OPENPMD_ADIOS2_STATS_LEVEL", 0);
        m_IO.SetParameter("StatsLevel", std::to_string(stats_level));
    }
    if (m_impl->realEngineType() == "sst" && notYetConfigured("QueueLimit"))
    {
        /*
         * By default, the SST engine of ADIOS2 does not set a limit on its
         * internal queue length.
         * If the reading end is slower than the writing end, this will
         * lead to a congestion in the queue and hence an increasing
         * memory usage while the writing code goes forward.
         * We could set a default queue limit of 1, thus forcing the
         * two codes to proceed entirely in lock-step.
         * We prefer a default queue limit of 2, which is still lower than
         * the default infinity, but allows writer and reader to process
         * data asynchronously as long as neither code fails to keep up the
         * rhythm. The writer can produce the next iteration while the
         * reader still deals with the old one.
         * Thus, a limit of 2 is a good balance between 1 and infinity,
         * keeping pipeline parallelism a default without running the risk
         * of using unbound memory.
         */
        m_IO.SetParameter("QueueLimit", "2");
    }

    // We need to open the engine now already to inquire configuration
    // options stored in there
    getEngine();
}

auto ADIOS2File::detectGroupTable() -> UseGroupTable
{
    auto const &attributes = availableAttributes();
    auto lower_bound =
        attributes.lower_bound(adios_defaults::str_activeTablePrefix);
    if (lower_bound != attributes.end() &&
        auxiliary::starts_with(
            lower_bound->first, adios_defaults::str_activeTablePrefix))
    {
        return UseGroupTable::Yes;
    }
    else
    {
        return UseGroupTable::No;
    }
}

adios2::Engine &ADIOS2File::getEngine()
{
    if (!m_engine)
    {
        auto tempMode = m_mode;
        switch (m_mode)
        {
        case adios2::Mode::Append:
#ifdef _WIN32
            /*
             * On Windows, ADIOS2 v2.8. Append mode only works with existing
             * files. So, we first check for file existence and switch to
             * create mode if it does not exist.
             *
             * See issue: https://github.com/ornladios/ADIOS2/issues/3358
             */
            tempMode = m_impl->checkFile(m_file) ? adios2::Mode::Append
                                                 : adios2::Mode::Write;
            [[fallthrough]];
#endif
        case adios2::Mode::Write: {
            // usesSteps attribute only written upon ::advance()
            // this makes sure that the attribute is only put in case
            // the streaming API was used.
            m_engine =
                std::make_optional(adios2::Engine(m_IO.Open(m_file, tempMode)));
            m_engine->BeginStep();
            streamStatus = StreamStatus::DuringStep;
            break;
        }
#if openPMD_HAS_ADIOS_2_8
        case adios2::Mode::ReadRandomAccess:
#endif
        case adios2::Mode::Read: {
            m_engine =
                std::make_optional(adios2::Engine(m_IO.Open(m_file, m_mode)));
            /*
             * First round: detect use of group table
             */
            bool openedANewStep = false;
            {
                if (!supportsUpfrontParsing(
                        m_impl->m_handler->m_backendAccess,
                        m_impl->m_engineType))
                {
                    /*
                     * In BP5 with Linear read mode, we now need to
                     * tentatively open the first IO step.
                     * Otherwise we don't see the group table attributes.
                     * This branch is also taken by Streaming engines.
                     */
                    if (m_engine->BeginStep() != adios2::StepStatus::OK)
                    {
                        throw std::runtime_error(
                            "[ADIOS2] Unexpected step status when "
                            "opening file/stream.");
                    }
                    openedANewStep = true;
                }

                if (m_impl->m_useGroupTable.has_value())
                {
                    switch (m_impl->m_useGroupTable.value())
                    {
                    case UseGroupTable::Yes: {
                        auto detectedGroupTable = detectGroupTable();
                        if (detectedGroupTable == UseGroupTable::No)
                        {
                            std::cerr
                                << "[Warning] User requested use of group "
                                   "table when reading from ADIOS2 "
                                   "dataset, but no group table has been "
                                   "found. Will ignore."
                                << std::endl;
                            m_impl->m_useGroupTable = UseGroupTable::No;
                        }
                    }
                    case UseGroupTable::No:
                        break;
                    }
                }
                else
                {
                    m_impl->m_useGroupTable = detectGroupTable();
                }
            };

            /*
             * Second round: Decide the streamStatus.
             */
            switch (streamStatus)
            {
            case StreamStatus::Undecided: {
                auto attr = m_IO.InquireAttribute<bool_representation>(
                    adios_defaults::str_usesstepsAttribute);
                if (attr && attr.Data()[0] == 1)
                {
                    if (parsePreference == ParsePreference::UpFront)
                    {
                        if (openedANewStep)
                        {
                            throw error::Internal(
                                "Logic error in ADIOS2 backend! No need to "
                                "indiscriminately open a step before doing "
                                "anything in an engine that supports "
                                "up-front parsing.");
                        }
                        streamStatus = StreamStatus::ReadWithoutStream;
                    }
                    else
                    {
                        // If the iteration encoding is group-based and
                        // no group table is used, we're now at a dead-end.
                        // Step-by-Step parsing is unreliable in that mode
                        // since groups might be reported that are not
                        // there.
                        // But we were only able to find this out by opening
                        // the ADIOS2 file with an access mode that was
                        // possibly wrong, so we would have to close and
                        // reopen here.
                        // Since group-based encoding is a bag of trouble in
                        // ADIOS2 anyway, we just don't support this
                        // particular use case.
                        // This failure will only arise when the following
                        // conditions are met:
                        //
                        // 1) group-based encoding
                        // 2) no group table (i.e. old "ADIOS2 schema")
                        // 3) LINEAR access mode
                        //
                        // This is a relatively lenient restriction compared
                        // to forbidding group-based encoding in ADIOS2
                        // altogether.
                        if (m_impl->m_useGroupTable.value() ==
                                UseGroupTable::No &&
                            m_IO.InquireAttribute<std::string>(
                                adios_defaults::str_groupBasedWarning))
                        {
                            throw error::OperationUnsupportedInBackend(
                                "ADIOS2",
                                "Trying to open a group-based ADIOS2 file "
                                "that does not have a group table with "
                                "LINEAR access type. That combination is "
                                "very buggy, so please use "
                                "READ_ONLY/READ_RANDOM_ACCESS instead.");
                        }
                        if (!openedANewStep &&
                            m_engine.value().BeginStep() !=
                                adios2::StepStatus::OK)
                        {
                            throw std::runtime_error(
                                "[ADIOS2] Unexpected step status when "
                                "opening file/stream.");
                        }
                        streamStatus = StreamStatus::DuringStep;
                    }
                }
                else
                {
                    /*
                     * If openedANewStep is true, then the file consists
                     * of one large step, we just leave it open.
                     */
                    streamStatus = StreamStatus::ReadWithoutStream;
                }
                break;
            }
            case StreamStatus::ReadWithoutStream:
                // using random-access mode
                break;
            case StreamStatus::DuringStep:
                throw error::Internal(
                    "[ADIOS2] Control flow error: stream status cannot be "
                    "DuringStep before opening the engine.");
            case StreamStatus::OutsideOfStep:
                if (openedANewStep)
                {
                    streamStatus = StreamStatus::DuringStep;
                }
                else
                {
                    throw error::Internal(
                        "Control flow error: Step should have been opened "
                        "before this point.");
                }
                break;
            default:
                throw std::runtime_error("[ADIOS2] Control flow error!");
            }
            break;
        }
        default:
            throw std::runtime_error("[ADIOS2] Invalid ADIOS access mode");
        }

        if (!m_engine.value())
        {
            throw std::runtime_error("[ADIOS2] Failed opening Engine.");
        }
    }
    return m_engine.value();
}

void ADIOS2File::flush_impl(
    ADIOS2FlushParams flushParams,
    std::function<void(ADIOS2File &, adios2::Engine &)> const &performPutGets,
    bool writeLatePuts,
    bool flushUnconditionally)
{
    auto level = flushParams.level;
    if (streamStatus == StreamStatus::StreamOver)
    {
        if (flushUnconditionally)
        {
            throw std::runtime_error(
                "[ADIOS2] Cannot access engine since stream is over.");
        }
        return;
    }
    auto &eng = getEngine();
    /*
     * Only open a new step if it is necessary.
     */
    if (streamStatus == StreamStatus::OutsideOfStep)
    {
        if (m_buffer.empty() && (!writeLatePuts || m_uniquePtrPuts.empty()))
        {
            if (flushUnconditionally)
            {
                performPutGets(*this, eng);
            }
            return;
        }
    }
    for (auto &ba : m_buffer)
    {
        ba->run(*this);
    }

    if (!initializedDefaults)
    {
        // Currently only schema 0 supported
        if (m_impl->m_writeAttributesFromThisRank)
        {
            m_IO.DefineAttribute<uint64_t>(adios_defaults::str_adios2Schema, 0);
        }
        initializedDefaults = true;
    }

    if (writeLatePuts)
    {
        for (auto &entry : m_uniquePtrPuts)
        {
            entry.run(*this);
        }
    }

    if (readOnly(m_mode))
    {
        level = FlushLevel::UserFlush;
    }

    switch (level)
    {
    case FlushLevel::UserFlush:
        performPutGets(*this, eng);
        m_updateSpans.clear();
        m_buffer.clear();
        m_alreadyEnqueued.clear();
        if (writeLatePuts)
        {
            m_uniquePtrPuts.clear();
        }

        break;

    case FlushLevel::InternalFlush:
    case FlushLevel::SkeletonOnly:
    case FlushLevel::CreateOrOpenFiles:
        /*
         * Tasks have been given to ADIOS2, but we don't flush them
         * yet. So, move everything to m_alreadyEnqueued to avoid
         * use-after-free.
         */
        for (auto &task : m_buffer)
        {
            m_alreadyEnqueued.emplace_back(std::move(task));
        }
        if (writeLatePuts)
        {
            throw error::Internal(
                "ADIOS2 backend: Flush of late writes was requested at the "
                "wrong time.");
        }
        m_buffer.clear();
        break;
    }
}

void ADIOS2File::flush_impl(ADIOS2FlushParams flushParams, bool writeLatePuts)
{
    auto decideFlushAPICall = [this, flushTarget = flushParams.flushTarget](
                                  adios2::Engine &engine) {
#if ADIOS2_VERSION_MAJOR * 1000000000 + ADIOS2_VERSION_MINOR * 100000000 +     \
        ADIOS2_VERSION_PATCH * 1000000 + ADIOS2_VERSION_TWEAK >=               \
    2701001223
        enum class CleanedFlushTarget
        {
            Buffer,
            Disk,
            Step
        };

        CleanedFlushTarget target{};
        switch (flushTarget)
        {
        case FlushTarget::Disk:
        case FlushTarget::Disk_Override:
            if (m_impl->realEngineType() == "bp5" ||
                /* this second check should be sufficient, but we leave the
                   first check in as a safeguard against renamings in
                   ADIOS2. Also do a lowerCase transform since the docstring
                   of `Engine::Type()` claims that the return value is in
                   lowercase, but for BP5 this does not seem true. */
                auxiliary::lowerCase(engine.Type()) == "bp5writer")
            {
                target = CleanedFlushTarget::Disk;
            }
            else
            {
                target = CleanedFlushTarget::Buffer;
            }
            break;
        case FlushTarget::Buffer:
        case FlushTarget::Buffer_Override:
            target = CleanedFlushTarget::Buffer;
            break;
        case FlushTarget::NewStep:
        case FlushTarget::NewStep_Override:
            target = CleanedFlushTarget::Step;
            break;
        }

        switch (target)
        {
        case CleanedFlushTarget::Disk:
            /*
             * Draining the uniquePtrPuts now to use this chance to free the
             * memory.
             */
            for (auto &entry : m_uniquePtrPuts)
            {
                entry.run(*this);
            }
            engine.PerformDataWrite();
            m_uniquePtrPuts.clear();
            m_updateSpans.clear();
            break;
        case CleanedFlushTarget::Buffer:
            engine.PerformPuts();
            break;
        case CleanedFlushTarget::Step:
            if (streamStatus != StreamStatus::DuringStep)
            {
                throw error::OperationUnsupportedInBackend(
                    "ADIOS2",
                    "Trying to flush to a new step while no step is active");
            }
            /*
             * Draining the uniquePtrPuts now to use this chance to free the
             * memory.
             */
            for (auto &entry : m_uniquePtrPuts)
            {
                entry.run(*this);
            }
            engine.EndStep();
            engine.BeginStep();
            // ++m_currentStep; // think we should keep this as the logical step
            m_uniquePtrPuts.clear();
            uncommittedAttributes.clear();
            m_updateSpans.clear();
            break;
        }
#else
        (void)this;
        (void)flushTarget;
        engine.PerformPuts();
#endif
    };

    flush_impl(
        flushParams,
        [decideFlushAPICall = std::move(decideFlushAPICall)](
            ADIOS2File &ba, adios2::Engine &eng) {
            if (writeOnly(ba.m_mode))
            {
                decideFlushAPICall(eng);
            }
            else
            {
                eng.PerformGets();
            }
        },
        writeLatePuts,
        /* flushUnconditionally = */ false);
}

AdvanceStatus ADIOS2File::advance(AdvanceMode mode)
{
    if (streamStatus == StreamStatus::Undecided)
    {
        throw error::Internal(
            "[ADIOS2File::advance()] StreamStatus Undecided before "
            "beginning/ending a step?");
    }
    // sic! no else
    if (streamStatus == StreamStatus::ReadWithoutStream)
    {
        flush(
            ADIOS2FlushParams{FlushLevel::UserFlush},
            /* writeLatePuts = */ false);
        return AdvanceStatus::RANDOMACCESS;
    }

    switch (mode)
    {
    case AdvanceMode::ENDSTEP: {
        /*
         * Advance mode write:
         * Close the current step, defer opening the new step
         * until one is actually needed:
         * (1) The engine is accessed in ADIOS2File::flush
         * (2) A new step is opened before the currently active step
         *     has seen an access. See the following lines: open the
         *     step just to skip it again.
         */
        if (streamStatus == StreamStatus::OutsideOfStep)
        {
            if (getEngine().BeginStep() != adios2::StepStatus::OK)
            {
                throw std::runtime_error(
                    "[ADIOS2] Trying to close a step that cannot be "
                    "opened.");
            }
        }

        if (writeOnly(m_mode) && m_impl->m_writeAttributesFromThisRank &&
            !m_IO.InquireAttribute<bool_representation>(
                adios_defaults::str_usesstepsAttribute))
        {
            m_IO.DefineAttribute<bool_representation>(
                adios_defaults::str_usesstepsAttribute, 1);
        }

        flush(
            ADIOS2FlushParams{FlushLevel::UserFlush},
            [](ADIOS2File &, adios2::Engine &eng) { eng.EndStep(); },
            /* writeLatePuts = */ true,
            /* flushUnconditionally = */ true);
        uncommittedAttributes.clear();
        m_updateSpans.clear();
        streamStatus = StreamStatus::OutsideOfStep;
        ++m_currentStep;
        return AdvanceStatus::OK;
    }
    case AdvanceMode::BEGINSTEP: {
        adios2::StepStatus adiosStatus{};

        if (streamStatus != StreamStatus::DuringStep)
        {
            adiosStatus = getEngine().BeginStep();
        }
        else
        {
            adiosStatus = adios2::StepStatus::OK;
        }
        AdvanceStatus res = AdvanceStatus::OK;
        switch (adiosStatus)
        {
        case adios2::StepStatus::EndOfStream:
            streamStatus = StreamStatus::StreamOver;
            res = AdvanceStatus::OVER;
            break;
        case adios2::StepStatus::OK:
            streamStatus = StreamStatus::DuringStep;
            res = AdvanceStatus::OK;
            break;
        case adios2::StepStatus::NotReady:
        case adios2::StepStatus::OtherError:
            throw std::runtime_error("[ADIOS2] Unexpected step status.");
        }
        invalidateAttributesMap();
        invalidateVariablesMap();
        m_pathsMarkedAsActive.clear();
        return res;
    }
    }
    throw std::runtime_error(
        "Internal error: Advance mode should be explicitly"
        " chosen by the front-end.");
}

void ADIOS2File::drop()
{
    assert(m_buffer.empty());
}

static std::vector<std::string> availableAttributesOrVariablesPrefixed(
    std::string const &prefix,
    ADIOS2File::AttributeMap_t const &(ADIOS2File::*getBasicMap)(),
    ADIOS2File &ba)
{
    std::string var = auxiliary::ends_with(prefix, '/') ? prefix : prefix + '/';
    ADIOS2File::AttributeMap_t const &attributes = (ba.*getBasicMap)();
    std::vector<std::string> ret;
    for (auto it = attributes.lower_bound(prefix); it != attributes.end(); ++it)
    {
        if (auxiliary::starts_with(it->first, var))
        {
            ret.emplace_back(auxiliary::replace_first(it->first, var, ""));
        }
        else
        {
            break;
        }
    }
    return ret;
}

std::vector<std::string>
ADIOS2File::availableAttributesPrefixed(std::string const &prefix)
{
    return availableAttributesOrVariablesPrefixed(
        prefix, &ADIOS2File::availableAttributes, *this);
}

std::vector<std::string>
ADIOS2File::availableVariablesPrefixed(std::string const &prefix)
{
    return availableAttributesOrVariablesPrefixed(
        prefix, &ADIOS2File::availableVariables, *this);
}

void ADIOS2File::invalidateAttributesMap()
{
    m_availableAttributes = std::optional<AttributeMap_t>();
}

ADIOS2File::AttributeMap_t const &ADIOS2File::availableAttributes()
{
    if (m_availableAttributes)
    {
        return m_availableAttributes.value();
    }
    else
    {
        m_availableAttributes = std::make_optional(m_IO.AvailableAttributes());
        return m_availableAttributes.value();
    }
}

void ADIOS2File::invalidateVariablesMap()
{
    m_availableVariables = std::optional<AttributeMap_t>();
}

ADIOS2File::AttributeMap_t const &ADIOS2File::availableVariables()
{
    if (m_availableVariables)
    {
        return m_availableVariables.value();
    }
    else
    {
        m_availableVariables = std::make_optional(m_IO.AvailableVariables());
        return m_availableVariables.value();
    }
}

void ADIOS2File::markActive(Writable *writable)
{
    switch (useGroupTable())
    {
    case UseGroupTable::No:
        break;
    case UseGroupTable::Yes:
#if openPMD_HAS_ADIOS_2_9
    {
        if (writeOnly(m_mode) && m_impl->m_writeAttributesFromThisRank)
        {
            auto currentStepBuffered = currentStep();
            do
            {
                using attr_t = unsigned long long;
                auto filePos = m_impl->setAndGetFilePosition(
                    writable, /* write = */ false);
                auto fullPath =
                    adios_defaults::str_activeTablePrefix + filePos->location;
                m_IO.DefineAttribute<attr_t>(
                    fullPath,
                    currentStepBuffered,
                    /* variableName = */ "",
                    /* separator = */ "/",
                    /* allowModification = */ true);
                m_pathsMarkedAsActive.emplace(writable);
                writable = writable->parent;
            } while (writable &&
                     m_pathsMarkedAsActive.find(writable) ==
                         m_pathsMarkedAsActive.end());
        }
    }
#else
        (void)writable;
        throw error::OperationUnsupportedInBackend(
            m_impl->m_handler->backendName(),
            "Group table feature requires ADIOS2 >= v2.9.");
#endif
    break;
    }
}
} // namespace openPMD::detail
#endif
