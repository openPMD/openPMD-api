/* Copyright 2026 Franz Poeschel
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "openPMD/config.hpp"

#if openPMD_HAVE_AWS

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
#endif
