#pragma once

#include <optional>
#include <string>

namespace openPMD
{
class ExternalBlockStorage;
}

namespace openPMD::internal
{
struct StdioBuilder
{
    std::string m_directory;
    std::optional<std::string> m_openMode = std::nullopt;

    auto setDirectory(std::string directory) -> StdioBuilder &;
    auto setOpenMode(std::string openMode) -> StdioBuilder &;

    operator ::openPMD::ExternalBlockStorage();
    auto build() -> ::openPMD::ExternalBlockStorage;
};
} // namespace openPMD::internal
