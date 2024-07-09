#pragma once

/* Copyright 2021-2023 Franz Poeschel
 *
 * This file is part of PIConGPU.
 *
 * PIConGPU is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PIConGPU is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PIConGPU.
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

    Pattern(std::string const &pattern_in, nlohmann::json config_in)
        // we construct the patterns once and use them often, so let's ask for
        // some optimization
        : pattern{pattern_in, std::regex_constants::egrep | std::regex_constants::optimize}
        , config{std::move(config_in)}
    {}
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
     * and extended configurations as defined by PIConGPU.
     * If an ordinary JSON configuration was detected, given regex
     * patterns will be matched against "" (the empty string).
     *
     * @param config The JSON configuration for one backend.
     *               E.g. for ADIOS2, this will be the sub-object/array found
     * under config["adios2"]["dataset"].
     */
    MatcherPerBackend(std::string backendName_in, TracingJSON config);

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
 * @brief Class to handle extended JSON configurations as used by
 *        the openPMD plugin.
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
     * @brief Initialize JSON matcher from command line arguments.
     *
     * This constructor will parse the given config, after reading it
     * from a file if needed. In this case, the constructor is
     * MPI-collective.
     * It will distinguish between ordinary openPMD JSON configurations
     * and extended configurations as defined by PIConGPU.
     * If an ordinary JSON configuration was detected, given regex
     * patterns will be matched against "" (the empty string).
     *
     * @param config The JSON configuration, exactly as in
     *               --openPMD.json.
     * @param comm MPI communicator for collective file reading,
     *             if needed.
     */
    JsonMatcher(openPMD::json::TracingJSON);

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
