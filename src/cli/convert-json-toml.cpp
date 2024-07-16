#include <cstdlib>
#include <cstring>
#include <openPMD/auxiliary/JSON_internal.hpp>

#include <iostream>
#include <sstream>
#include <string>

namespace json = openPMD::json;

void parsed_main(std::string jsonOrToml)
{
    auto [config, originallySpecifiedAs] = json::parseOptions(
        jsonOrToml, /* considerFiles = */ true, /* convertLowercase = */ false);
    {
        [[maybe_unused]] auto _ = std::move(jsonOrToml);
    }
    switch (originallySpecifiedAs)
    {
        using SL = json::SupportedLanguages;
    case SL::JSON: {
        auto asToml = json::jsonToToml(config);
        std::cout << asToml;
    }
    break;
    case SL::TOML:
        std::cout << config << '\n';
        break;
    }
}

int main(int argc, char const **argv)
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
            std::cout << "Usage: " << std::string(argv[0]) << R"( [json_or_toml]
'json_or_toml' can be a JSON or TOML dataset specified inline or a reference
to a file prepended by an '@'.
Inline datasets will be interpreted as JSON if they start with an '{', as TOML
otherwise. Datasets from a file will be interpreted as JSON or TOML depending
on the file ending '.json' or '.toml' respectively.
Inline dataset specifications can be replaced by input read from stdin.

If the input is JSON, then it will be converted to TOML and written to stdout,
equivalently from TOML to JSON.
)";
            exit(0);
        }
        jsonOrToml = argv[1];
        break;
    default:
        throw std::runtime_error(
            std::string("Usage: ") + argv[0] +
            " [file location or inline JSON/TOML]");
    }
    parsed_main(std::move(jsonOrToml));
}
