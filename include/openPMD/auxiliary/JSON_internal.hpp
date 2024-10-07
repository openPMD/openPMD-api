/* Copyright 2020-2021 Franz Poeschel
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

#include "openPMD/config.hpp"

#include <nlohmann/json.hpp>
#include <toml.hpp>

#include <optional>

#if openPMD_HAVE_MPI
#include <mpi.h>
#endif

#include <memory> // std::shared_ptr
#include <utility> // std::forward

namespace openPMD
{
namespace json
{
    enum class SupportedLanguages
    {
        JSON,
        TOML
    };

    struct ParsedConfig
    {
        nlohmann::json config;
        SupportedLanguages originallySpecifiedAs{SupportedLanguages::JSON};
    };

    /**
     * @brief Extend nlohmann::json with tracing of which keys have been
     * accessed by operator[]().
     * An access is only registered if the current JSON value is a JSON object
     * (not an array) and if the accessed JSON value is a leaf, i.e. anything
     * but an object. This means that objects contained in arrays will not be
     * traced.
     *
     * If working directly with the underlying JSON value (necessary since this
     * class only redefines operator[]), declareFullyRead() may be used to
     * declare keys read manually.
     *
     */
    class TracingJSON
    {
    public:
        TracingJSON();
        TracingJSON(nlohmann::json, SupportedLanguages);
        TracingJSON(ParsedConfig);

        /**
         * @brief Access the underlying JSON value
         *
         * @return nlohmann::json&
         */
        nlohmann::json &json();

        /**
         * @brief Access the underlying JSON value
         *
         * See args, first arg, used to distinguish this overload.
         *
         * @param path Index to some sub-expression. Shortcut for
         *        `tracingJSON[arg1][arg2][arg3].json()`.
         * @return nlohmann::json&
         */
        nlohmann::json &json(std::vector<std::string> path);

        template <typename Key>
        TracingJSON operator[](Key &&key);

        /**
         * @brief Get the "shadow", i.e. a copy of the original JSON value
         * containing all accessed object keys.
         *
         * @return nlohmann::json const&
         */
        nlohmann::json const &getShadow() const;
        nlohmann::json &getShadow();

        /**
         * @brief Invert the "shadow", i.e. a copy of the original JSON value
         * that contains exactly those values that have not been accessed yet.
         *
         * @return nlohmann::json
         */
        nlohmann::json invertShadow() const;

        /**
         * @brief Declare all keys of the current object read.
         *
         * Rationale: This class does not (yet) trace array types (or anything
         * contained in an array). Use this call to explicitly declare
         * an array as read.
         */
        void declareFullyRead();

        SupportedLanguages originallySpecifiedAs{SupportedLanguages::JSON};

    private:
        /**
         * @brief The JSON object with which this class has been initialized.
         *        Shared pointer shared between all instances returned by
         *        operator[]() in order to avoid use-after-free situations.
         *
         */
        std::shared_ptr<nlohmann::json> m_originalJSON;
        /**
         * @brief A JSON object keeping track of all accessed indices within the
         *        original JSON object. Initially an empty JSON object,
         *        gradually filled by applying each operator[]() call also to
         *        it.
         *        Shared pointer shared between all instances returned by
         *        operator[]() in order to avoid use-after-free situations.
         *
         */
        std::shared_ptr<nlohmann::json> m_shadow;
        /**
         * @brief The sub-expression within m_originalJSON corresponding with
         *        the current instance.
         *
         */
        nlohmann::json *m_positionInOriginal;
        /**
         * @brief The sub-expression within m_positionInOriginal corresponding
         *        with the current instance.
         *
         */
        nlohmann::json *m_positionInShadow;
        bool m_trace = true;

        void invertShadow(
            nlohmann::json &result, nlohmann::json const &shadow) const;

        TracingJSON(
            std::shared_ptr<nlohmann::json> originalJSON,
            std::shared_ptr<nlohmann::json> shadow,
            nlohmann::json *positionInOriginal,
            nlohmann::json *positionInShadow,
            SupportedLanguages originallySpecifiedAs,
            bool trace);
    };

    template <typename Key>
    TracingJSON TracingJSON::operator[](Key &&key)
    {
        nlohmann::json *newPositionInOriginal =
            &m_positionInOriginal->operator[](key);
        // If accessing a leaf in the JSON tree from an object (not an array!)
        // erase the corresponding key
        static nlohmann::json nullvalue;
        nlohmann::json *newPositionInShadow = &nullvalue;
        if (m_trace && m_positionInOriginal->is_object())
        {
            newPositionInShadow = &m_positionInShadow->operator[](key);
        }
        bool traceFurther = newPositionInOriginal->is_object();
        return TracingJSON(
            m_originalJSON,
            m_shadow,
            newPositionInOriginal,
            newPositionInShadow,
            originallySpecifiedAs,
            traceFurther);
    }

    nlohmann::json tomlToJson(toml::value const &val);
    toml::value jsonToToml(nlohmann::json const &val);

    /**
     * Check if options points to a file (indicated by an '@' for the first
     * non-whitespace character).
     * If yes, return the file content, if not just parse options directly.
     *
     * @param options as a parsed JSON object.
     * @param considerFiles If yes, check if `options` refers to a file and read
     *        from there.
     */
    ParsedConfig parseOptions(std::string const &options, bool considerFiles);

#if openPMD_HAVE_MPI

    /**
     * Parallel version of parseOptions(). MPI-collective.
     */
    ParsedConfig
    parseOptions(std::string const &options, MPI_Comm comm, bool considerFiles);

#endif

    /**
     * Recursively transform all keys in a JSON dataset to lower case.
     * String values are unaffected.
     * JSON objects at the following openPMD-defined locations are not affected:
     * * `adios2.engine.parameters`
     * * `adios2.dataset.operators.<number>.parameters`
     * This helps us forward configurations from these locations to ADIOS2
     * "as-is".
     */
    nlohmann::json &lowerCase(nlohmann::json &);

    /**
     * Read a JSON literal as a string.
     * If the literal is a number, convert that number to its string
     * representation.
     * If it is a bool, convert it to either "0" or "1".
     * If it is not a literal, return an empty option.
     */
    std::optional<std::string> asStringDynamic(nlohmann::json const &);

    /**
     * Like asStringDynamic(), but convert the string to lowercase afterwards.
     */
    std::optional<std::string> asLowerCaseStringDynamic(nlohmann::json const &);

    /**
     * Vector containing the lower-case keys to the single backends'
     * configurations.
     */
    extern std::vector<std::string> backendKeys();

    /**
     * Function that can be called after reading all global options from the
     * JSON configuration (i.e. all the options that are not handled by the
     * single backends).
     * If any unread value persists, a warning is printed to stderr.
     */
    void warnGlobalUnusedOptions(TracingJSON const &config);

    /**
     * Like merge() as defined in JSON.hpp, but this overload works directly
     * on nlohmann::json values.
     */
    nlohmann::json &
    merge(nlohmann::json &defaultVal, nlohmann::json const &overwrite);

    nlohmann::json &filterByTemplate(
        nlohmann::json &defaultVal, nlohmann::json const &positiveMask);

    template <typename toml_t>
    std::string format_toml(toml_t &&);
} // namespace json
} // namespace openPMD
