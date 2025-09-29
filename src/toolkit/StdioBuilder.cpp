#include "openPMD/toolkit/StdioBuilder.hpp"

#include "openPMD/toolkit/ExternalBlockStorage.hpp"
#include "openPMD/toolkit/Stdio.hpp"

#include <memory>

namespace openPMD::internal
{
auto StdioBuilder::setDirectory(std::string directory) -> StdioBuilder &
{
    m_directory = std::move(directory);
    return *this;
}
auto StdioBuilder::setOpenMode(std::string openMode) -> StdioBuilder &
{
    m_openMode = std::move(openMode);
    return *this;
}

StdioBuilder::operator ExternalBlockStorage()
{
    return ExternalBlockStorage{std::make_unique<ExternalBlockStorageStdio>(
        std::move(m_directory), std::move(m_openMode).value_or("wb"))};
}

auto StdioBuilder::build() -> ExternalBlockStorage
{
    return *this;
}
} // namespace openPMD::internal
