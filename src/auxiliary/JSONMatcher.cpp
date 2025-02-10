#include "openPMD/auxiliary/JSONMatcher.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"

#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace openPMD::json
{
// Anonymous namespace so these helpers don't get exported
namespace
{
    /**
     * @brief Read a single JSON pattern of the form {"select": ..., "cfg": ...}
     *
     * The "select" key is optional, indicating the default configuration if it
     * is missing.
     *
     * @param backend_name For error messages.
     * @param index_in_list For error messages.
     * @param patterns Output parameter: Emplace a parsed pattern into this
     * list.
     * @param defaultConfig Output parameter: If the pattern was the default
     * pattern, emplace it here.
     * @param object The JSON object that is parsed as the pattern.
     * @return Whether the pattern was the default configuration or not.
     */
    auto readPattern(
        std::string const &backend_name,
        size_t index_in_list,
        std::vector<Pattern> &patterns,
        std::optional<nlohmann::json> &defaultConfig,
        nlohmann::json object) -> void;
} // namespace

Pattern::Pattern(std::string const &pattern_in, nlohmann::json config_in)
    : config(std::move(config_in))
{
    // transform the regex such that the path to the Iteration is optional
    std::stringstream build_pattern;
    build_pattern << "(/data/[0-9]+/)?(" << pattern_in << ")";
    // we construct the patterns once and use them often, so let's ask for
    // some optimization
    pattern = std::regex(
        build_pattern.str(),
        std::regex_constants::egrep | std::regex_constants::optimize);
}

void MatcherPerBackend::init(TracingJSON tracing_config)
{
    auto &config = tracing_config.json();
    if (config.is_object())
    {
        return;
    }
    else if (config.is_array())
    {
        m_patterns.reserve(config.size());
        std::optional<nlohmann::json> defaultConfig;
        // enhanced PIConGPU-defined layout
        for (size_t i = 0; i < config.size(); ++i)
        {
            readPattern(
                backendName,
                i,
                m_patterns,
                defaultConfig,
                std::move(config.at(i)));
        }
        // now replace the pattern list with the default config
        tracing_config.json() =
            std::move(defaultConfig).value_or(nlohmann::json::object());
    }
    else
    {
        throw error::BackendConfigSchema(
            {backendName, "dataset"}, "Expecting an object or an array.");
    }
}

/**
 * @brief Get the JSON config associated with a regex pattern.
 *
 * @param datasetPath The regex.
 * @return The matched JSON configuration, as a string.
 */
nlohmann::json const &
MatcherPerBackend::get(std::string const &datasetPath) const
{
    for (auto const &pattern : m_patterns)
    {
        if (std::regex_match(datasetPath, pattern.pattern))
        {
            return pattern.config;
        }
    }
    static nlohmann::json const emptyConfig; // null
    return emptyConfig;
}

auto JsonMatcher::init() -> void
{
    // Copy required since the config will be modified
    if (!m_entireConfig.json().is_object())
    {
        throw error::BackendConfigSchema(
            {}, "Expected an object for the JSON configuration.");
    }
}

MatcherPerBackend::MatcherPerBackend() = default;

MatcherPerBackend::MatcherPerBackend(
    std::string backendName_in, TracingJSON config)
    : backendName(std::move(backendName_in))
{
    init(std::move(config));
}

JsonMatcher::JsonMatcher() = default;

JsonMatcher::JsonMatcher(TracingJSON entireConfig)
    : m_entireConfig(std::move(entireConfig))
{
    init();
}

auto JsonMatcher::get(
    std::string const &datasetPath, std::string const &backendName)
    -> ParsedConfig
{
    initBackendLazily(backendName);

    nlohmann::json result = nlohmann::json::object();
    // might not have been initialized due to unspecified configuration
    if (m_backendMatcher.backendName == backendName)
    {
        auto const &datasetConfig = m_backendMatcher.get(datasetPath);
        if (!datasetConfig.empty())
        {
            result[backendName]["dataset"] = datasetConfig;
        }
    }

    return {result, m_entireConfig.originallySpecifiedAs};
}

auto JsonMatcher::getDefault(std::string const &backendName) -> TracingJSON
{
    initBackendLazily(backendName);
    return m_entireConfig;
}

auto JsonMatcher::initBackendLazily(std::string const &backendName) -> void
{
    if (m_backendMatcher.backendName == backendName)
    {
        // already initialized
        return;
    }
    if (!m_entireConfig.json().contains(backendName))
    {
        return;
    }
    auto const &backendConfig = m_entireConfig.json({backendName});
    if (!backendConfig.is_object())
    {
        throw error::BackendConfigSchema(
            {backendName},
            "Each backend's configuration must be a JSON object (config "
            "for backend " +
                backendName + ").");
    }
    else if (!backendConfig.contains("dataset"))
    {
        return;
    }
    m_backendMatcher =
        MatcherPerBackend(backendName, m_entireConfig[backendName]["dataset"]);
}

namespace
{
    auto readPattern(
        std::string const &backend_name,
        size_t index_in_list,
        std::vector<Pattern> &patterns,
        std::optional<nlohmann::json> &defaultConfig,
        nlohmann::json object) -> void
    {
        constexpr char const *errorMsg = R"END(
Each single pattern in an dataset-specific JSON/TOML configuration must be
an object with mandatory key 'cfg' and optional key 'select'.
When the key 'select' is not specified, the given configuration is used
for setting up the default dataset configuration upon backend initialization.
The key 'select' must point to either a single string or an array of strings
and is interpreted as a regular expression against which the dataset name
(full path or path within an iteration) must match.)END";
        auto throw_up = [&](std::string const &additional_info,
                            auto &&...additional_path) {
            throw error::BackendConfigSchema(
                {backend_name,
                 "dataset",
                 std::to_string(index_in_list),
                 additional_path...},
                additional_info + errorMsg);
        };

        if (!object.is_object())
        {
            throw_up("Not an object!");
        }
        if (!object.contains("cfg"))
        {
            throw_up("Mandatory key missing: 'cfg'!");
        }
        {
            std::vector<std::string> unrecognized_keys;
            for (auto it = object.begin(); it != object.end(); ++it)
            {
                if (it.key() == "select" || it.key() == "cfg")
                {
                    continue;
                }
                unrecognized_keys.emplace_back(it.key());
            }
            if (!unrecognized_keys.empty())
            {
                std::cerr << "[Warning] JSON/TOML config at '" << backend_name
                          << ".dataset." << index_in_list
                          << "' has unrecognized keys:";
                for (auto const &item : unrecognized_keys)
                {
                    std::cerr << " '" << item << '\'';
                }
                std::cerr << '.' << std::endl;
            }
        }

        nlohmann::json &cfg = object.at("cfg");
        if (!object.contains("select"))
        {
            if (defaultConfig.has_value())
            {
                throw_up("Specified more than one default configuration!");
            }
            defaultConfig.emplace(std::move(cfg));
            return;
        }
        else
        {
            nlohmann::json const &pattern = object.at("select");
            std::string pattern_str = [&]() -> std::string {
                if (pattern.is_string())
                {
                    return pattern.get<std::string>();
                }
                else if (pattern.is_array())
                {
                    std::stringstream res;
                    res << "($^)";
                    for (auto const &sub_pattern : pattern)
                    {
                        if (!sub_pattern.is_string())
                        {
                            throw_up(
                                "Must be a string or an array of string!",
                                "select");
                        }
                        res << "|(" << sub_pattern.get<std::string>() << ")";
                    }
                    return res.str();
                }
                else
                {
                    throw_up(
                        "Must be a string or an array of string!", "select");
                    throw std::runtime_error("Unreachable!");
                }
            }();
            patterns.emplace_back(pattern_str, std::move(cfg));
            return;
        }
    }
} // namespace
} // namespace openPMD::json
