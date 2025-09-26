#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <string>

namespace openPMD
{
class ExternalBlockStorage;
}

namespace openPMD::internal
{
struct ExternalBlockStorageBackend
{
    virtual auto
    put(std::string const &identifier, void const *data, size_t len)
        -> std::string = 0;
    virtual ~ExternalBlockStorageBackend();
};

struct StdioBuilder
{
    std::string m_directory;
    std::optional<std::string> m_openMode = std::nullopt;

    auto setDirectory(std::string directory) -> StdioBuilder &;
    auto setOpenMode(std::string openMode) -> StdioBuilder &;

    operator ExternalBlockStorage();
    auto build() -> ExternalBlockStorage;
};
} // namespace openPMD::internal

namespace openPMD
{
class ExternalBlockStorage
{
private:
    std::unique_ptr<internal::ExternalBlockStorageBackend> m_worker;
    ExternalBlockStorage(
        std::unique_ptr<internal::ExternalBlockStorageBackend>);

    friend struct internal::StdioBuilder;

public:
    static auto makeStdioSession(std::string directory)
        -> internal::StdioBuilder;
};
} // namespace openPMD
