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

public:
    ExternalBlockStorageAws(Aws::S3::S3Client, std::string bucketName);
    auto put(std::string const &identifier, void const *data, size_t len)
        -> std::string override;
    ~ExternalBlockStorageAws() override;
};
} // namespace openPMD::internal
