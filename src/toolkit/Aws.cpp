#include "openPMD/toolkit/Aws.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/auxiliary/Memory_internal.hpp"
#include "openPMD/auxiliary/Variant.hpp"

#include <aws/s3/S3Client.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <iostream>
#include <mutex>
#include <stdexcept>

namespace
{
struct membuf : std::streambuf
{
    membuf(char const *base, std::size_t size)
    {
        auto p = const_cast<char *>(base);
        this->setg(p, p, p + size);
    }
};

struct imemstream : std::iostream
{
    imemstream(char const *base, std::size_t size)
        : std::iostream(&m_buf), m_buf(base, size)
    {}

private:
    membuf m_buf;
};
} // namespace

namespace openPMD::internal
{
void AwsAsyncCounter::wait()
{
    size_t target = this->request_counter;
    std::unique_lock lk(this->mutex);
    this->event.wait(
        lk, [this, target]() { return this->completion_counter >= target; });
}

void AwsAsyncCounter::add_task()
{
    this->request_counter++;
}

void AwsAsyncCounter::add_and_notify_result()
{
    std::unique_lock lk(this->mutex);
    this->completion_counter++;
    lk.unlock();
    this->event.notify_all();
}

AwsAsyncCounter::~AwsAsyncCounter()
{
    std::cerr << "Waiting for remaining tasks. Have " << completion_counter
              << " of " << request_counter << std::endl;
    this->wait();
    std::cerr << "Finished waiting for remaining tasks" << std::endl;
}

ExternalBlockStorageAws::ExternalBlockStorageAws(
    Aws::S3::S3Client client,
    std::string bucketName,
    std::optional<std::string> endpoint,
    bool async)
    : m_client{std::move(client)}
    , m_bucketName(std::move(bucketName))
    , m_endpoint(std::move(endpoint))
    , m_async(async ? std::make_optional<AwsAsyncHandler>() : std::nullopt)
{
    Aws::S3::Model::CreateBucketRequest create_request;
    create_request.SetBucket(m_bucketName);
    auto create_outcome = m_client.CreateBucket(create_request);
    if (!create_outcome.IsSuccess())
    {
        std::cerr << "[ExternalBlockStorageAws::ExternalBlockStorageAws] "
                     "Warning: Failed to create bucket (may already exist): "
                  << create_outcome.GetError().GetMessage() << std::endl;
    }
    else
    {
        std::cout << "Bucket created: " << m_bucketName << std::endl;
    }
}
ExternalBlockStorageAws::~ExternalBlockStorageAws()
{
    // We need to wait for late operations before doing anything else.
    m_async.reset();
}

auto ExternalBlockStorageAws::put(
    std::string const &identifier, auxiliary::WriteBuffer data, size_t len)
    -> std::string
{
    auto sanitized = !identifier.empty() && identifier.at(0) == '/'
        ? identifier.substr(1)
        : identifier;

    Aws::S3::Model::PutObjectRequest put_request;
    put_request.SetBucket(m_bucketName);
    put_request.SetKey(sanitized);

    auto input_data = Aws::MakeShared<imemstream>(
        "PutObjectInputStream",
        reinterpret_cast<char const *>(data.get()),
        len);
    put_request.SetBody(input_data);
    put_request.SetContentLength(static_cast<long long>(len));

    if (!m_async.has_value())
    {
        auto put_outcome = m_client.PutObject(put_request);

        if (put_outcome.IsSuccess())
        {
            // std::cout << "File synchronously uploaded successfully to S3!"
            //           << std::endl;
        }
        else
        {
            std::cerr << "Synchronous upload failed: "
                      << put_outcome.GetError().GetMessage() << std::endl;
        }
    }
    else
    {
        auto &async_counter = *std::visit(
            auxiliary::overloaded{
                [this](auxiliary::WriteBuffer::CopyableUniquePtr const &) {
                    std::cout << "Using unique pointer" << std::endl;
                    return &this->m_async->unique_ptr_operations;
                },
                [this](auxiliary::WriteBuffer::SharedPtr const &) {
                    std::cout << "Using shared pointer" << std::endl;
                    return &this->m_async->shared_ptr_operations;
                }},
            data.as_variant<auxiliary::WriteBufferTypes>());
        auto responseReceivedHandler =
            [&async_counter,
             sanitized,
             /*
              * Need to keep buffers alive until they have been asynchronously
              * read. Use the closure captures for this. Wrap the WriteBuffer
              * inside a shared_ptr to make the std::function copyable.
              */
             keepalive =
                 std::make_shared<auxiliary::WriteBuffer>(std::move(data))](
                const Aws::S3::S3Client *,
                const Aws::S3::Model::PutObjectRequest &,
                const Aws::S3::Model::PutObjectOutcome &put_outcome,
                const std::shared_ptr<const Aws::Client::AsyncCallerContext>
                    &) {
                (void)keepalive;
                if (put_outcome.IsSuccess())
                {
                    // std::cout
                    //     << "File asynchronously uploaded successfully to S3!"
                    //     << std::endl;
                }
                else
                {
                    std::cerr << "Asynchronous upload failed for '" << sanitized
                              << "': " << put_outcome.GetError().GetMessage()
                              << std::endl;
                }
                async_counter.add_and_notify_result();
            };
        async_counter.add_task();
        m_client.PutObjectAsync(put_request, responseReceivedHandler);
    }
    return sanitized;
}

void ExternalBlockStorageAws::get(
    std::string const &external_ref, void *data, size_t len)
{
    if (len == 0)
    {
        return;
    }

    Aws::S3::Model::GetObjectRequest get_request;
    get_request.SetBucket(m_bucketName);
    get_request.SetKey(external_ref);

    auto get_outcome = m_client.GetObject(get_request);
    if (!get_outcome.IsSuccess())
    {
        throw std::runtime_error(
            std::string("ExternalBlockStorageAws::get failed: ") +
            get_outcome.GetError().GetMessage());
    }

    auto &body = get_outcome.GetResult().GetBody();
    body.read(
        reinterpret_cast<char *>(data), static_cast<std::streamsize>(len));
    std::streamsize read_bytes = body.gcount();
    if (read_bytes != static_cast<std::streamsize>(len))
    {
        throw std::runtime_error(
            "ExternalBlockStorageAws: failed to read expected number of bytes "
            "from S3 object");
    }
}

void ExternalBlockStorageAws::sync()
{
    if (!this->m_async.has_value())
    {
        return;
    }
    this->m_async->shared_ptr_operations.wait();
}

[[nodiscard]] auto ExternalBlockStorageAws::externalStorageLocation() const
    -> nlohmann::json
{
    nlohmann::json j;
    j["provider"] = "aws";
    if (m_endpoint.has_value())
    {
        j["endpoint"] = *m_endpoint;
    }
    j["bucket"] = m_bucketName;
    return j;
}

} // namespace openPMD::internal
