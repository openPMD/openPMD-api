#include "openPMD/toolkit/Aws.hpp"

#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <iostream>
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
ExternalBlockStorageAws::ExternalBlockStorageAws(
    Aws::S3::S3Client client,
    std::string bucketName,
    std::optional<std::string> endpoint)
    : m_client{std::move(client)}
    , m_bucketName(std::move(bucketName))
    , m_endpoint(std::move(endpoint))
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
ExternalBlockStorageAws::~ExternalBlockStorageAws() = default;

auto ExternalBlockStorageAws::put(
    std::string const &identifier, void const *data, size_t len) -> std::string
{
    auto sanitized = !identifier.empty() && identifier.at(0) == '/'
        ? identifier.substr(1)
        : identifier;

    Aws::S3::Model::PutObjectRequest put_request;
    put_request.SetBucket(m_bucketName);
    put_request.SetKey(sanitized);

    auto input_data = Aws::MakeShared<imemstream>(
        "PutObjectInputStream", reinterpret_cast<char const *>(data), len);
    put_request.SetBody(input_data);
    put_request.SetContentLength(static_cast<long long>(len));

    auto put_outcome = m_client.PutObject(put_request);

    if (put_outcome.IsSuccess())
    {
        std::cout << "File uploaded successfully to S3!" << std::endl;
    }
    else
    {
        std::cerr << "Upload failed: " << put_outcome.GetError().GetMessage()
                  << std::endl;
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
