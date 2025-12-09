#pragma once

#include <cstdlib>
#include <cstring>
#include <openPMD/auxiliary/JSON_internal.hpp>

#include <iostream>
#include <sstream>
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
    static void with_parsed_cmdline_args(std::string jsonOrToml)
    {
        namespace json = openPMD::json;
        auto [config, originallySpecifiedAs] = json::parseOptions(
            jsonOrToml,
            /* considerFiles = */ true,
            /* convertLowercase = */ false);
        {
            // NOLINTNEXTLINE(bugprone-unused-local-non-trivial-variable)
            [[maybe_unused]] auto _ = std::move(jsonOrToml);
        }
        switch (originallySpecifiedAs)
        {
            using SL = json::SupportedLanguages;
        case SL::JSON: {
            auto asToml = json::jsonToToml(config);
            std::cout << json::format_toml(asToml);
        }
        break;
        case SL::TOML:
            std::cout << config << '\n';
            break;
        }
    }

public:
    static void run_application(
        int argc, char const **argv, void (*print_help_message)(char const *))
    {
        std::string jsonOrToml;
        switch (argc)
        {
        case 0:
        case 1:
            // Just read the whole stream into memory
            // Not very elegant, but we'll hold the entire JSON/TOML dataset
            // in memory at some point anyway, so it doesn't really matter
            {
                std::stringbuf readEverything;
                std::cin >> &readEverything;
                jsonOrToml = readEverything.str();
            }
            break;
        case 2:
            if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
            {
                print_help_message(argv[1]);
                exit(0);
            }
            jsonOrToml = argv[1];
            break;
        default:
            throw std::runtime_error(
                std::string("Usage: ") + argv[0] +
                " [file location or inline JSON/TOML]");
        }
        with_parsed_cmdline_args(std::move(jsonOrToml));
    }
};
