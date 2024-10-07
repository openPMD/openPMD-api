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

#include "openPMD/auxiliary/JSON.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"

#include "openPMD/Error.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/Variant.hpp"

#include <limits>
#include <queue>
#include <toml.hpp>

#include <algorithm>
#include <cctype> // std::isspace
#include <fstream>
#include <iostream> // std::cerr
#include <map>
#include <optional>
#include <sstream>
#include <utility> // std::forward
#include <vector>

namespace openPMD::json
{
TracingJSON::TracingJSON() : TracingJSON(ParsedConfig{})
{}

TracingJSON::TracingJSON(
    nlohmann::json originalJSON, SupportedLanguages originallySpecifiedAs_in)
    : originallySpecifiedAs(originallySpecifiedAs_in)
    , m_originalJSON(std::make_shared<nlohmann::json>(std::move(originalJSON)))
    , m_shadow(std::make_shared<nlohmann::json>())
    , m_positionInOriginal(&*m_originalJSON)
    , m_positionInShadow(&*m_shadow)
{}

TracingJSON::TracingJSON(ParsedConfig parsedConfig)
    : TracingJSON{
          std::move(parsedConfig.config), parsedConfig.originallySpecifiedAs}
{}

nlohmann::json &TracingJSON::json()
{
    return *m_positionInOriginal;
}

nlohmann::json &TracingJSON::json(std::vector<std::string> paths)
{
    if (paths.empty())
    {
        return json();
    }
    auto it = paths.begin();
    auto end = paths.end();
    nlohmann::json *res = &m_positionInOriginal->operator[](*it);
    auto subhandle = this->operator[](*it);
    for (++it; it != end; ++it)
    {
        subhandle = subhandle[*it];
        res = &(*res)[*it];
    }
    return *res;
}

nlohmann::json const &TracingJSON::getShadow() const
{
    return *m_positionInShadow;
}

nlohmann::json &TracingJSON::getShadow()
{
    return *m_positionInShadow;
}

nlohmann::json TracingJSON::invertShadow() const
{
    nlohmann::json inverted = *m_positionInOriginal;
    invertShadow(inverted, *m_positionInShadow);
    return inverted;
}

void TracingJSON::invertShadow(
    nlohmann::json &result, nlohmann::json const &shadow) const
{
    if (!shadow.is_object())
    {
        return;
    }
    std::vector<std::string> toRemove;
    for (auto it = shadow.begin(); it != shadow.end(); ++it)
    {
        auto partialResultIterator = result.find(it.key());
        if (partialResultIterator == result.end())
        {
            // The shadow contained a key that was not in the original dataset
            continue;
        }
        nlohmann::json &partialResult = partialResultIterator.value();
        if (partialResult.is_object())
        {
            invertShadow(partialResult, it.value());
            if (partialResult.size() == 0)
            {
                toRemove.emplace_back(it.key());
            }
        }
        else
        {
            toRemove.emplace_back(it.key());
        }
    }
    for (auto const &key : toRemove)
    {
        result.erase(key);
    }
}

void TracingJSON::declareFullyRead()
{
    if (m_trace)
    {
        // copy over
        *m_positionInShadow = *m_positionInOriginal;
    }
}

TracingJSON::TracingJSON(
    std::shared_ptr<nlohmann::json> originalJSON,
    std::shared_ptr<nlohmann::json> shadow,
    nlohmann::json *positionInOriginal,
    nlohmann::json *positionInShadow,
    SupportedLanguages originallySpecifiedAs_in,
    bool trace)
    : originallySpecifiedAs(originallySpecifiedAs_in)
    , m_originalJSON(std::move(originalJSON))
    , m_shadow(std::move(shadow))
    , m_positionInOriginal(positionInOriginal)
    , m_positionInShadow(positionInShadow)
    , m_trace(trace)
{}

namespace
{
    std::optional<std::string> extractFilename(std::string const &unparsed)
    {
        std::string trimmed =
            auxiliary::trim(unparsed, [](char c) { return std::isspace(c); });
        if (!trimmed.empty() && trimmed.at(0) == '@')
        {
            trimmed = trimmed.substr(1);
            trimmed = auxiliary::trim(
                trimmed, [](char c) { return std::isspace(c); });
            return std::make_optional(trimmed);
        }
        else
        {
            return std::optional<std::string>{};
        }
    }

    nlohmann::json
    tomlToJson(toml::value const &val, std::vector<std::string> &currentPath);

    nlohmann::json
    tomlToJson(toml::value const &val, std::vector<std::string> &currentPath)
    {
        if (val.is_boolean())
        {
            return val.as_boolean();
        }
        else if (val.is_integer())
        {
            return val.as_integer();
        }
        else if (val.is_floating())
        {
            return val.as_floating();
        }
        else if (val.is_string())
        {
            return std::string(val.as_string());
        }
        else if (
            val.is_offset_datetime() || val.is_local_datetime() ||
            val.is_local_date() || val.is_local_time())
        {
            throw error::BackendConfigSchema(
                currentPath, "Cannot convert date/time type to JSON.");
        }
        else if (val.is_array())
        {
            auto const &arr = val.as_array();
            nlohmann::json result = nlohmann::json::array();
            for (size_t i = 0; i < arr.size(); ++i)
            {
                currentPath.push_back(std::to_string(i));
                result[i] = tomlToJson(arr[i], currentPath);
                currentPath.pop_back();
            }
            return result;
        }
        else if (val.is_table())
        {
            auto const &tab = val.as_table();
            nlohmann::json result = nlohmann::json::object();
            for (auto const &pair : tab)
            {
                currentPath.push_back(pair.first);
                result[pair.first] = tomlToJson(pair.second, currentPath);
                currentPath.pop_back();
            }
            return result;
        }

        // @todo maybe generalize error type
        throw error::BackendConfigSchema(
            currentPath,
            "Unexpected datatype in TOML configuration. This is probably a "
            "bug.");
    }

    toml::value jsonToToml(
        nlohmann::json const &val, std::vector<std::string> &currentPath);

    toml::value
    jsonToToml(nlohmann::json const &val, std::vector<std::string> &currentPath)
    {
        switch (val.type())
        {
        case nlohmann::json::value_t::null:
            throw error::BackendConfigSchema(
                currentPath, "TOML does not support null values.");
        case nlohmann::json::value_t::object: {
            toml::value::table_type res;
            for (auto pair = val.begin(); pair != val.end(); ++pair)
            {
                currentPath.push_back(pair.key());
                res[pair.key()] = jsonToToml(pair.value(), currentPath);
                currentPath.pop_back();
            }
            return toml::value(res);
        }
        case nlohmann::json::value_t::array: {
            toml::value::array_type res;
            res.reserve(val.size());
            size_t index = 0;
            for (auto const &entry : val)
            {
                currentPath.push_back(std::to_string(index));
                res.emplace_back(jsonToToml(entry, currentPath));
                currentPath.pop_back();
            }
            return toml::value(res);
        }
        case nlohmann::json::value_t::string:
            return val.get<std::string>();
        case nlohmann::json::value_t::boolean:
            return val.get<bool>();
        case nlohmann::json::value_t::number_integer:
            return val.get<nlohmann::json::number_integer_t>();
        case nlohmann::json::value_t::number_unsigned:
            return val.get<nlohmann::json::number_unsigned_t>();
        case nlohmann::json::value_t::number_float:
            return (long double)val.get<nlohmann::json::number_float_t>();
        case nlohmann::json::value_t::binary:
            return val.get<nlohmann::json::binary_t>();
        case nlohmann::json::value_t::discarded:
            throw error::BackendConfigSchema(
                currentPath,
                "Internal JSON parser datatype leaked into JSON value.");
        }
        throw std::runtime_error("Unreachable!");
    }
} // namespace

nlohmann::json tomlToJson(toml::value const &val)
{
    std::vector<std::string> currentPath;
    // that's as deep as our config currently goes, +1 for good measure
    currentPath.reserve(7);
    return tomlToJson(val, currentPath);
}

toml::value jsonToToml(nlohmann::json const &val)
{
    std::vector<std::string> currentPath;
    // that's as deep as our config currently goes, +1 for good measure
    currentPath.reserve(7);
    return jsonToToml(val, currentPath);
}

namespace
{
    ParsedConfig parseInlineOptions(std::string const &options)
    {
        // speed up default options
        ParsedConfig res;
        if (options.empty())
        {
            res.originallySpecifiedAs = SupportedLanguages::TOML;
            res.config = nlohmann::json::object();
            return res;
        }
        else if (options == "{}")
        {
            res.originallySpecifiedAs = SupportedLanguages::JSON;
            res.config = nlohmann::json::object();
            return res;
        }
        std::string trimmed =
            auxiliary::trim(options, [](char c) { return std::isspace(c); });
        if (trimmed.empty())
        {
            return res;
        }
        if (trimmed.at(0) == '{')
        {
            res.config = nlohmann::json::parse(options);
            res.originallySpecifiedAs = SupportedLanguages::JSON;
        }
        else
        {
            std::istringstream istream(
                options.c_str(), std::ios_base::binary | std::ios_base::in);
            toml::value tomlVal =
                toml::parse(istream, "[inline TOML specification]");
            res.config = json::tomlToJson(tomlVal);
            res.originallySpecifiedAs = SupportedLanguages::TOML;
        }
        lowerCase(res.config);
        return res;
    }
} // namespace

ParsedConfig parseOptions(std::string const &options, bool considerFiles)
{
    if (considerFiles)
    {
        auto filename = extractFilename(options);
        if (filename.has_value())
        {
            std::fstream handle;
            handle.open(
                filename.value(), std::ios_base::binary | std::ios_base::in);
            ParsedConfig res;
            if (auxiliary::ends_with(filename.value(), ".toml"))
            {
                toml::value tomlVal = toml::parse(handle, filename.value());
                res.config = tomlToJson(tomlVal);
                res.originallySpecifiedAs = SupportedLanguages::TOML;
            }
            else
            {
                // default: JSON
                handle >> res.config;
                res.originallySpecifiedAs = SupportedLanguages::JSON;
            }
            if (!handle.good())
            {
                throw std::runtime_error(
                    "Failed reading JSON config from file " + filename.value() +
                    ".");
            }
            lowerCase(res.config);
            return res;
        }
    }
    return parseInlineOptions(options);
}

#if openPMD_HAVE_MPI
ParsedConfig
parseOptions(std::string const &options, MPI_Comm comm, bool considerFiles)
{
    if (considerFiles)
    {
        auto filename = extractFilename(options);
        if (filename.has_value())
        {
            ParsedConfig res;
            std::string fileContent =
                auxiliary::collective_file_read(filename.value(), comm);
            if (auxiliary::ends_with(filename.value(), ".toml"))
            {
                std::istringstream istream(
                    fileContent.c_str(),
                    std::ios_base::binary | std::ios_base::in);
                res.config = tomlToJson(toml::parse(istream, filename.value()));
                res.originallySpecifiedAs = SupportedLanguages::TOML;
            }
            else
            {
                // default:: JSON
                res.config = nlohmann::json::parse(fileContent);
                res.originallySpecifiedAs = SupportedLanguages::JSON;
            }
            lowerCase(res.config);
            return res;
        }
    }
    return parseInlineOptions(options);
}
#endif

template <typename F>
static nlohmann::json &lowerCase(
    nlohmann::json &json,
    std::vector<std::string> &currentPath,
    F const &ignoreCurrentPath)
{
    auto transFormCurrentObject = [&currentPath](
                                      nlohmann::json::object_t &val) {
        // somekey -> SomeKey
        std::map<std::string, std::string> originalKeys;
        for (auto &pair : val)
        {
            std::string lower = auxiliary::lowerCase(std::string(pair.first));
            auto findEntry = originalKeys.find(lower);
            if (findEntry != originalKeys.end())
            {
                // double entry found
                std::vector<std::string> copyCurrentPath{currentPath};
                copyCurrentPath.push_back(lower);
                throw error::BackendConfigSchema(
                    std::move(copyCurrentPath), "JSON config: duplicate keys.");
            }
            originalKeys.emplace_hint(findEntry, std::move(lower), pair.first);
        }

        nlohmann::json::object_t newObject;
        for (auto &pair : originalKeys)
        {
            newObject[pair.first] = std::move(val[pair.second]);
        }
        val = newObject;
    };

    if (json.is_object())
    {
        auto &val = json.get_ref<nlohmann::json::object_t &>();

        if (!ignoreCurrentPath(currentPath))
        {
            transFormCurrentObject(val);
        }

        // now recursively
        for (auto &pair : val)
        {
            // ensure that the path consists only of lowercase strings,
            // even if ignoreCurrentPath() was true
            currentPath.push_back(
                auxiliary::lowerCase(std::string(pair.first)));
            lowerCase(pair.second, currentPath, ignoreCurrentPath);
            currentPath.pop_back();
        }
    }
    else if (json.is_array())
    {
        for (auto &val : json)
        {
            currentPath.emplace_back("\vnum");
            lowerCase(val, currentPath, ignoreCurrentPath);
            currentPath.pop_back();
        }
    }
    return json;
}

nlohmann::json &lowerCase(nlohmann::json &json)
{
    std::vector<std::string> currentPath;
    // that's as deep as our config currently goes, +1 for good measure
    currentPath.reserve(7);
    return lowerCase(
        json, currentPath, [](std::vector<std::string> const &path) {
            std::vector<std::string> const ignoredPaths[] = {
                {"adios2", "engine", "parameters"},
                {"adios2",
                 "dataset",
                 "operators",
                 /*
                  * We use "\vnum" to indicate "any array index".
                  */
                 "\vnum",
                 "parameters"}};
            for (auto const &ignored : ignoredPaths)
            {
                if (ignored == path)
                {
                    return true;
                }
            }
            return false;
        });
}

std::optional<std::string> asStringDynamic(nlohmann::json const &value)
{
    if (value.is_string())
    {
        return value.get<std::string>();
    }
    else if (value.is_number_integer())
    {
        return std::to_string(value.get<long long>());
    }
    else if (value.is_number_float())
    {
        return std::to_string(value.get<long double>());
    }
    else if (value.is_boolean())
    {
        return std::string(value.get<bool>() ? "1" : "0");
    }
    return std::optional<std::string>{};
}

std::optional<std::string> asLowerCaseStringDynamic(nlohmann::json const &value)
{
    auto maybeString = asStringDynamic(value);
    if (maybeString.has_value())
    {
        auxiliary::lowerCase(maybeString.value());
    }
    return maybeString;
}

std::vector<std::string> backendKeys()
{
    return {"adios2", "json", "toml", "hdf5"};
}

void warnGlobalUnusedOptions(TracingJSON const &config)
{
    auto shadow = config.invertShadow();
    // The backends are supposed to deal with this
    // Only global options here
    for (auto const &backendKey : json::backendKeys())
    {
        shadow.erase(backendKey);
    }
    if (shadow.size() > 0)
    {
        switch (config.originallySpecifiedAs)
        {
        case SupportedLanguages::JSON:
            std::cerr
                << "[Series] The following parts of the global JSON config "
                   "remains unused:\n"
                << shadow.dump() << std::endl;
            break;
        case SupportedLanguages::TOML: {
            auto asToml = jsonToToml(shadow);
            std::cerr
                << "[Series] The following parts of the global TOML config "
                   "remains unused:\n"
                << json::format_toml(asToml) << std::endl;
        }
        }
    }
}

nlohmann::json &
merge(nlohmann::json &defaultVal, nlohmann::json const &overwrite)
{
    if (defaultVal.is_object() && overwrite.is_object())
    {
        std::queue<std::string> prunedKeys;
        for (auto it = overwrite.begin(); it != overwrite.end(); ++it)
        {
            auto &valueInDefault = defaultVal[it.key()];
            merge(valueInDefault, it.value());
            if (valueInDefault.is_null())
            {
                prunedKeys.push(it.key());
            }
        }
        for (; !prunedKeys.empty(); prunedKeys.pop())
        {
            defaultVal.erase(prunedKeys.front());
        }
    }
    else
    {
        /*
         * Anything else, just overwrite.
         * Note: There's no clear generic way to merge arrays:
         * Should we concatenate? Or should we merge at the same indices?
         * From the user side, this means:
         * An application can specify a number of default compression
         * operators, e.g. in adios2.dataset.operators, but a user can
         * overwrite the operators. Neither appending nor pointwise update
         * are quite useful here.
         */
        defaultVal = overwrite;
    }
    return defaultVal;
}

std::string merge(std::string const &defaultValue, std::string const &overwrite)
{
    auto [res, returnFormat] =
        parseOptions(defaultValue, /* considerFiles = */ false);
    merge(res, parseOptions(overwrite, /* considerFiles = */ false).config);
    switch (returnFormat)
    {
    case SupportedLanguages::JSON:
        return res.dump();
        break;
    case SupportedLanguages::TOML: {
        auto asToml = json::jsonToToml(res);
        std::stringstream sstream;
        sstream << json::format_toml(asToml);
        return sstream.str();
    }
    }
    throw std::runtime_error("Unreachable!");
}

nlohmann::json &
filterByTemplate(nlohmann::json &defaultVal, nlohmann::json const &positiveMask)
{
    if (defaultVal.is_object() && positiveMask.is_object())
    {
        std::queue<std::string> prunedKeys;
        for (auto left_it = defaultVal.begin(); left_it != defaultVal.end();
             ++left_it)
        {
            if (auto right_it = positiveMask.find(left_it.key());
                right_it != positiveMask.end())
            {
                // value is covered by mask, keep it
                filterByTemplate(left_it.value(), right_it.value());
            }
            else
            {
                prunedKeys.push(left_it.key());
            }
        }
        for (; !prunedKeys.empty(); prunedKeys.pop())
        {
            defaultVal.erase(prunedKeys.front());
        }
    } // else noop
    return defaultVal;
}

constexpr int toml_precision = std::numeric_limits<double>::digits10 + 1;

#if TOML11_VERSION_MAJOR < 4
template <typename toml_t>
std ::string format_toml(toml_t &&val)
{
    std::stringstream res;
    res << std::setprecision(toml_precision) << std::forward<toml_t>(val);
    return res.str();
}

#else

namespace
{
    auto set_precision(toml::value &) -> void;
    auto set_precision(toml::value &val) -> void
    {
        if (val.is_table())
        {
            for (auto &pair : val.as_table())
            {
                set_precision(pair.second);
            }
        }
        else if (val.is_array())
        {
            for (auto &entry : val.as_array())
            {
                set_precision(entry);
            }
        }
        else if (val.is_floating())
        {
            val.as_floating_fmt().prec = toml_precision;
        }
    }
} // namespace

template <typename toml_t>
std::string format_toml(toml_t &&val)
{
    set_precision(val);
    return toml::format(std::forward<toml_t>(val));
}

#endif

template std::string format_toml(toml::value &&);
template std::string format_toml(toml::value &);
} // namespace openPMD::json
