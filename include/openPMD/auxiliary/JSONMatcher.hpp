#pragma once

/* Copyright 2021-2024 Franz Poeschel
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

#include "openPMD/auxiliary/JSON_internal.hpp"

#include <regex>
#include <string>
#include <vector>

namespace openPMD::json
{
struct Pattern
{
    std::regex pattern;
    nlohmann::json config;

    Pattern(std::string const &pattern_in, nlohmann::json config_in);
};

/**
 * @brief Matcher for dataset configurations per backend.
 *
 */
class MatcherPerBackend
{
private:
    std::vector<Pattern> m_patterns;

    void init(TracingJSON config);

public:
    /**
     * @brief For default construction.
     */
    explicit MatcherPerBackend();

    /**
     * @brief Initialize one backend's JSON matcher from its configuration.
     *
     * This constructor will parse the given config.
     * It will distinguish between ordinary openPMD JSON configurations
     * and dataset-specific configurations.
     *
     * @param backendName The backend's JSON key.
     * @param config The JSON configuration for one backend.
     *               E.g. for ADIOS2, this will be the sub-object/array found
     * under config["adios2"]["dataset"].
     */
    MatcherPerBackend(std::string backendName, TracingJSON config);

    std::string backendName;

    /**
     * @brief Get the JSON config associated with a regex pattern.
     *
     * @param datasetPath The regex.
     * @return The matched JSON configuration, as a string.
     */
    auto get(std::string const &datasetPath) const -> nlohmann::json const &;
};
/**
 * @brief Class to handle default and dataset-specific JSON configurations.
 *
 * This class handles parsing of the extended JSON patterns as well as
 * selection of one JSON configuration by regex.
 *
 */
class JsonMatcher
{
private:
    std::vector<MatcherPerBackend> m_perBackend;
    TracingJSON m_entireConfig;

    auto init() -> void;

public:
    /**
     * @brief For default construction.
     */
    explicit JsonMatcher();

    /**
     * @brief Initialize JSON matcher from a parsed JSON config.
     *
     * Will go through the backends' configurations (keys defined by
     * `backendKeys` in JSON_internal.hpp) and check for dataset-specific
     * configurations. It will then construct:
     *
     * 1. A default configuration.
     * 2. Matchers for retrieving dataset-specific configurations.
     *
     * @param config The parsed JSON configuration as specified by the user.
     */
    JsonMatcher(openPMD::json::TracingJSON config);

    /**
     * @brief Get the JSON config associated with a regex pattern.
     *
     * @param datasetPath The regex.
     * @return The matched JSON configuration, as a string.
     */
    auto get(std::string const &datasetPath) const -> ParsedConfig;

    /**
     * @brief Get the default JSON config.
     *
     * @return The default JSON configuration.
     */
    auto getDefault() -> TracingJSON;
};
} // namespace openPMD::json
