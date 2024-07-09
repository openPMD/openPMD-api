#include "openPMD/auxiliary/JSONMatcher.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include <nlohmann/json.hpp>

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
     * @param patterns Output parameter: Emplace a parsed pattern into this
     * list.
     * @param defaultConfig Output parameter: If the pattern was the default
     * pattern, emplace it here.
     * @param object The JSON object that is parsed as the pattern.
     * @return Whether the pattern was the default configuration or not.
     */
    auto readPattern(
        std::vector<Pattern> &patterns,
        std::optional<nlohmann::json> &defaultConfig,
        nlohmann::json object) -> void;
} // namespace

void MatcherPerBackend::init(TracingJSON tracing_config)
{
    auto &config = tracing_config.json();
    if (config.is_object())
    {
        return;
    }
    else if (config.is_array())
    {
        std::optional<nlohmann::json> defaultConfig;
        // enhanced PIConGPU-defined layout
        for (auto &value : config)
        {
            readPattern(m_patterns, defaultConfig, std::move(value));
        }
        // now replace the pattern list with the default config
        tracing_config.json() =
            std::move(defaultConfig).value_or(nlohmann::json::object());
    }
    else
    {
        throw std::runtime_error(
            "[openPMD plugin] Expecting an object or an array as JSON "
            "configuration.");
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
    m_perBackend.reserve(backendKeys.size());
    for (auto it = m_entireConfig.json().begin();
         it != m_entireConfig.json().end();
         ++it)
    {
        std::string const &backendName = it.key();
        if (std::find(backendKeys.begin(), backendKeys.end(), backendName) ==
            backendKeys.end())
        {
            // The key does not point to the configuration of a backend
            // recognized by PIConGPU Ignore it.
            continue;
        }
        if (!it.value().is_object())
        {
            throw error::BackendConfigSchema(
                {it.key()},
                "Each backend's configuration must be a JSON object (config "
                "for backend " +
                    backendName + ").");
        }
        if (it.value().contains("dataset"))
        {
            m_perBackend.emplace_back(
                backendName, m_entireConfig[it.key()]["dataset"]);
        }
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

auto JsonMatcher::get(std::string const &datasetPath) const -> ParsedConfig
{
    nlohmann::json result = nlohmann::json::object();
    for (auto const &backend : m_perBackend)
    {
        auto const &datasetConfig = backend.get(datasetPath);
        if (datasetConfig.empty())
        {
            // ensure that there actually is an object to erase this from
            result[backend.backendName]["dataset"] = {};
            result[backend.backendName].erase("dataset");
        }
        else
        {
            result[backend.backendName]["dataset"] = datasetConfig;
        }
    }
    return {result, m_entireConfig.originallySpecifiedAs};
}

auto JsonMatcher::getDefault() -> TracingJSON
{
    return m_entireConfig;
}

namespace
{
    auto readPattern(
        std::vector<Pattern> &patterns,
        std::optional<nlohmann::json> &defaultConfig,
        nlohmann::json object) -> void
    {
        constexpr char const *errorMsg = &R"END(
Each single pattern in an extended JSON configuration must be a JSON object
with keys 'select' and 'cfg'.
The key 'select' is optional, indicating a default configuration if it is
not set.
The key 'select' must point to either a single string or an array of strings.)END"
                                             [1];

        if (!object.is_object())
        {
            throw std::runtime_error(errorMsg);
        }
        try
        {
            nlohmann::json &cfg = object.at("cfg");
            if (!object.contains("select"))
            {
                if (defaultConfig.has_value())
                {
                    throw std::runtime_error(
                        "Specified more than one default configuration.");
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
                            res << "|(" << sub_pattern.get<std::string>()
                                << ")";
                        }
                        return res.str();
                    }
                    else
                    {
                        throw std::runtime_error(errorMsg);
                    }
                }();
                patterns.emplace_back(pattern_str, std::move(cfg));
                return;
            }
        }
        catch (nlohmann::json::out_of_range const &)
        {
            throw std::runtime_error(errorMsg);
        }
    }
} // namespace
} // namespace openPMD::json
