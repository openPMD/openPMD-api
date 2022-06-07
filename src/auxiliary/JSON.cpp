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

#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/auxiliary/Option.hpp"
#include "openPMD/auxiliary/StringManip.hpp"

#include <cctype> // std::isspace
#include <fstream>
#include <sstream>
#include <vector>

namespace openPMD
{
namespace auxiliary
{
    TracingJSON::TracingJSON() : TracingJSON(nlohmann::json())
    {}

    TracingJSON::TracingJSON(nlohmann::json originalJSON)
        : m_originalJSON(
              std::make_shared<nlohmann::json>(std::move(originalJSON)))
        , m_shadow(std::make_shared<nlohmann::json>())
        , m_positionInOriginal(&*m_originalJSON)
        , m_positionInShadow(&*m_shadow)
    {}

    nlohmann::json const &TracingJSON::getShadow()
    {
        return *m_positionInShadow;
    }

    nlohmann::json TracingJSON::invertShadow()
    {
        nlohmann::json inverted = *m_positionInOriginal;
        invertShadow(inverted, *m_positionInShadow);
        return inverted;
    }

    void TracingJSON::invertShadow(
        nlohmann::json &result, nlohmann::json const &shadow)
    {
        if (!shadow.is_object())
        {
            return;
        }
        std::vector<std::string> toRemove;
        for (auto it = shadow.begin(); it != shadow.end(); ++it)
        {
            nlohmann::json &partialResult = result[it.key()];
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
        bool trace)
        : m_originalJSON(std::move(originalJSON))
        , m_shadow(std::move(shadow))
        , m_positionInOriginal(positionInOriginal)
        , m_positionInShadow(positionInShadow)
        , m_trace(trace)
    {}

    namespace
    {
        auxiliary::Option<std::string>
        extractFilename(std::string const &unparsed)
        {
            std::string trimmed = auxiliary::trim(
                unparsed, [](char c) { return std::isspace(c); });
            if (trimmed.at(0) == '@')
            {
                trimmed = trimmed.substr(1);
                trimmed = auxiliary::trim(
                    trimmed, [](char c) { return std::isspace(c); });
                return auxiliary::makeOption(trimmed);
            }
            else
            {
                return auxiliary::Option<std::string>{};
            }
        }
    } // namespace

    nlohmann::json parseOptions(std::string const &options)
    {
        auto filename = extractFilename(options);
        if (filename.has_value())
        {
            std::fstream handle;
            handle.open(filename.get(), std::ios_base::in);
            nlohmann::json res;
            handle >> res;
            if (!handle.good())
            {
                throw std::runtime_error(
                    "Failed reading JSON config from file " + filename.get() +
                    ".");
            }
            return res;
        }
        else
        {
            return nlohmann::json::parse(options);
        }
    }

#if openPMD_HAVE_MPI
    nlohmann::json parseOptions(std::string const &options, MPI_Comm comm)
    {
        auto filename = extractFilename(options);
        if (filename.has_value())
        {
            return nlohmann::json::parse(
                auxiliary::collective_file_read(filename.get(), comm));
        }
        else
        {
            return nlohmann::json::parse(options);
        }
    }
#endif
} // namespace auxiliary
} // namespace openPMD
