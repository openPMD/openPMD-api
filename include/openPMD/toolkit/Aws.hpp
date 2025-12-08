#pragma once

#include "openPMD/toolkit/ExternalBlockStorage.hpp"

#include <aws/s3/S3Client.h>

namespace openPMD::internal
{
struct ExternalBlockStorageAws : ExternalBlockStorageBackend
{
private:
    Aws::S3::S3Client m_client;
    std::string m_bucketName;
    std::optional<std::string> m_endpoint;

public:
    ExternalBlockStorageAws(
        Aws::S3::S3Client,
        std::string bucketName,
        std::optional<std::string> endpoint);
    auto put(std::string const &identifier, void const *data, size_t len)
        -> std::string override;
    void get(std::string const &external_ref, void *data, size_t len) override;
    [[nodiscard]] auto externalStorageLocation() const
        -> nlohmann::json override;
    ~ExternalBlockStorageAws() override;
};
} // namespace openPMD::internal
