#pragma once

#include "openPMD/toolkit/ExternalBlockStorage.hpp"

namespace openPMD::internal
{
struct ExternalBlockStorageStdio : ExternalBlockStorageBackend
{
private:
    std::string m_directory;
    std::string m_openMode;

public:
    ExternalBlockStorageStdio(std::string directory, std::string openMode);
    auto put(std::string const &identifier, void const *data, size_t len)
        -> std::string override;
    [[nodiscard]] auto externalStorageLocation() const
        -> nlohmann::json override;
    ~ExternalBlockStorageStdio() override;
};
} // namespace openPMD::internal
