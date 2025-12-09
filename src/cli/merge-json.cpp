#include "openPMD/cli/convert-toml-json.hpp"

void print_help_message(char const *program_name)
{
    std::cout << "Usage: " << std::string(program_name) << R"( [json_or_toml]+
'json_or_toml' can be a JSON or TOML dataset specified inline or a reference
to a file prepended by an '@'.
Inline datasets will be interpreted as JSON if they start with an '{', as TOML
otherwise. Datasets from a file will be interpreted as JSON or TOML depending
on the file ending '.json' or '.toml' respectively.
Inline dataset specifications can be replaced by input read from stdin.

If the JSON/TOML files are mixed, then the output type (JSON or TOML) will be
determined by the type of the first file.
)";
}

int main(int argc, char const **argv)
{
    convert_json_toml<from_format_to_format::ID>::run_application(
        argc, argv, print_help_message);
}
