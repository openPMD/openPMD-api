/* Copyright 2026 Franz Poeschel
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

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iterator>
#include <openPMD/auxiliary/JSON_internal.hpp>

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace from_format_to_format
{
namespace json = openPMD::json;
struct ID
{
    template <json::SupportedLanguages originallySpecifiedAs>
    static auto call(nlohmann::json const &&val)
    // template <>
    // auto call<json::SupportedLanguages::JSON>(nlohmann::json const &val) ->
    // nlohmann::json const&
    {
        if constexpr (originallySpecifiedAs == json::SupportedLanguages::JSON)
        {
            return val;
        }
        else
        {
            return json::jsonToToml(val);
        }
    }
};

struct switch_
{
    template <json::SupportedLanguages>
    struct other_type;
    template <json::SupportedLanguages originallySpecifiedAs>
    static auto call(nlohmann::json const &&val)
    {
        return ID::call<other_type<originallySpecifiedAs>::value>(
            std::move(val));
    }
};
template <>
struct switch_::other_type<json::SupportedLanguages::JSON>
{
    static constexpr json::SupportedLanguages value =
        json::SupportedLanguages::TOML;
};
template <>
struct switch_::other_type<json::SupportedLanguages::TOML>
{
    static constexpr json::SupportedLanguages value =
        json::SupportedLanguages::JSON;
};
} // namespace from_format_to_format

template <typename FromFormatToFormat>
class convert_json_toml
{
    static void print(toml::value &val)
    {
        namespace json = openPMD::json;
        std::cout << json::format_toml(val);
    }
    static void print(nlohmann::json const &val)
    {
        std::cout << val << '\n';
    }
    static void
    with_parsed_cmdline_args(openPMD::json::ParsedConfig parsed_config)
    {
        namespace json = openPMD::json;
        auto [config, originallySpecifiedAs] = std::move(parsed_config);
        switch (originallySpecifiedAs)
        {
            using SL = json::SupportedLanguages;
        case SL::JSON: {
            auto for_print =
                FromFormatToFormat::template call<SL::JSON>(std::move(config));
            print(for_print);
        }
        break;
        case SL::TOML: {
            auto for_print =
                FromFormatToFormat::template call<SL::TOML>(std::move(config));
            print(for_print);
        }
        break;
        }
    }

    struct ByLine : std::string
    {
        friend auto operator>>(std::istream &i, ByLine &l) -> std::istream &
        {
            decltype(auto) res = std::getline(i, l);
            if (res)
            {
                l.insert(0, 1, '@');
            }
            return res;
        }
    };
    using ByLineIterator = std::istream_iterator<ByLine>;

    template <typename It>
    static auto merge(It begin, It end) -> openPMD::json::ParsedConfig
    {
        namespace json = openPMD::json;
        if (begin == end)
        {
            throw std::runtime_error(
                "merge: need at least one JSON/TOML file.");
        }
        auto config = json::parseOptions(
            *begin,
            /* considerFiles = */ true,
            /* convertLowercase = */ false);
        for (++begin; begin != end; ++begin)
        {
            auto [next, _] = json::parseOptions(
                *begin,
                /* considerFiles = */ true,
                /* convertLowercase = */ false);
            json::merge_internal(config.config, next, /* do_prune = */ false);
        }
        return config;
    }

public:
    enum class UseStdinAs : std::uint8_t
    {
        InlineJson,
        ListOfJson
    };

    static void run_application(
        int argc,
        char const **argv,
        UseStdinAs stdinconfig,
        void (*print_help_message)(char const *))
    {
        std::string jsonOrToml;
        switch (argc)
        {
        case 0:
        case 1:
            switch (stdinconfig)
            {
            case UseStdinAs::InlineJson: {
                // Just read the whole stream into memory
                // Not very elegant, but we'll hold the entire JSON/TOML dataset
                // in memory at some point anyway, so it doesn't really matter
                std::stringbuf readEverything;
                std::cin >> &readEverything;
                jsonOrToml = readEverything.str();
                break;
            }
            case UseStdinAs::ListOfJson: {
                auto parsed_config =
                    merge(ByLineIterator(std::cin), ByLineIterator{});
                with_parsed_cmdline_args(std::move(parsed_config));
                break;
            }
            }
            break;
        default:
            if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
            {
                print_help_message(argv[1]);
                exit(0);
            }
            auto parsed_config = merge(argv + 1, argv + argc);
            with_parsed_cmdline_args(std::move(parsed_config));
            break;
        }
    }
};
