#pragma once

#include "openPMD/toolkit/ExternalBlockStorage.hpp"

#include <aws/s3/S3Client.h>

#include <condition_variable>

namespace openPMD::internal
{
struct AwsAsyncCounter
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

    ~AwsAsyncCounter();
};

struct AwsAsyncHandler
{
    // We can defer std::unique_ptr operations longer than std::shared_ptr
    // operations, since no one else has the memory, so use two counters. TODO:
    // Add some form of restriction on how long the std::unique_ptr queue may
    // become. Currently it can theoretically be spammed ad libitum. Either
    // restrict the queue to a configurable length, or add a syncEverything()
    // call.
    AwsAsyncCounter shared_ptr_operations, unique_ptr_operations;
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
    auto
    put(std::string const &identifier, auxiliary::WriteBuffer data, size_t len)
        -> std::string override;
    void
    get(std::string const &external_ref,
        std::shared_ptr<void> data,
        size_t len) override;
    [[nodiscard]] auto externalStorageLocation() const
        -> nlohmann::json override;
    void syncMandatoryOperations() override;
    void syncAllOperations() override;

    ~ExternalBlockStorageAws() override;
};
} // namespace openPMD::internal
