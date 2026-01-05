#pragma once

#include "openPMD/toolkit/ExternalBlockStorage.hpp"

#include <aws/s3/S3Client.h>

#include <condition_variable>

namespace openPMD::internal
{
struct AwsAsyncHandler
{
    std::mutex mutex;
    std::condition_variable event;
    std::size_t request_counter = 0;
    // Upon C++20, we can use a std::atomic for this and ditch the
    // condition_variable + mutex approach
    std::size_t completion_counter = 0;

    void wait();
    void add_task();
    void add_and_notify_result();

    ~AwsAsyncHandler();
};

struct ExternalBlockStorageAws : ExternalBlockStorageBackend
{
private:
    Aws::S3::S3Client m_client;
    std::string m_bucketName;
    std::optional<std::string> m_endpoint;
    std::optional<AwsAsyncHandler> m_async;

public:
    ExternalBlockStorageAws(
        Aws::S3::S3Client,
        std::string bucketName,
        std::optional<std::string> endpoint,
        bool async);
    auto put(std::string const &identifier, void const *data, size_t len)
        -> std::string override;
    void get(std::string const &external_ref, void *data, size_t len) override;
    [[nodiscard]] auto externalStorageLocation() const
        -> nlohmann::json override;
    void sync() override;

    ~ExternalBlockStorageAws() override;
};
} // namespace openPMD::internal
