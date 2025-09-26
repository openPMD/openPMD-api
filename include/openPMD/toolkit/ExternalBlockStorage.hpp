#pragma once

#include "openPMD/Dataset.hpp"

#include <nlohmann/json.hpp>

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
    explicit ExternalBlockStorage();

    static auto makeStdioSession(std::string directory)
        -> internal::StdioBuilder;

    // returns created JSON key
    template <typename T>
    auto store(
        Extent globalExtent,
        Offset blockOffset,
        Extent blockExtent,
        nlohmann::json &fullJsonDataset,
        nlohmann::json::json_pointer const &path,
        T const *data) -> std::string;

    static void sanitizeString(std::string &s);
};
} // namespace openPMD
