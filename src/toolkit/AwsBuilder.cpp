#include "openPMD/toolkit/AwsBuilder.hpp"

#include "openPMD/toolkit/Aws.hpp"
#include "openPMD/toolkit/ExternalBlockStorage.hpp"

#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/signer/AWSAuthV4Signer.h>
#include <aws/core/http/Scheme.h>

namespace openPMD::internal
{
AwsBuilder::AwsBuilder(
    std::string bucketName, std::string accessKeyId, std::string secretKey)
    : m_bucketName(std::move(bucketName))
    , m_accessKeyId(std::move(accessKeyId))
    , m_secretKey(std::move(secretKey))
{}

auto AwsBuilder::setBucketName(std::string bucketName) -> AwsBuilder &
{
    m_bucketName = std::move(bucketName);
    return *this;
}

auto internal::AwsBuilder::setCredentials(
    std::string accessKeyId, std::string secretKey) -> AwsBuilder &
{
    m_accessKeyId = std::move(accessKeyId);
    m_secretKey = std::move(secretKey);
    return *this;
}

auto AwsBuilder::setEndpointOverride(std::string endpoint) -> AwsBuilder &
{
    m_endpointOverride = std::move(endpoint);
    return *this;
}

auto AwsBuilder::setRegion(std::string regionName) -> AwsBuilder &
{
    m_region = std::move(regionName);
    return *this;
}

auto AwsBuilder::setScheme(Scheme s) -> AwsBuilder &
{
    m_scheme = s;
    return *this;
}

auto internal::AwsBuilder::setSessionToken(std::string sessionToken)
    -> AwsBuilder &
{
    m_sessionToken = std::move(sessionToken);
    return *this;
}

AwsBuilder::operator ExternalBlockStorage()
{
    Aws::Client::ClientConfiguration config;

    if (m_endpointOverride.has_value())
    {
        config.endpointOverride = *m_endpointOverride;
    }
    if (m_region.has_value())
    {
        config.region = *m_region;
    }
    else
    {
        config.region = "us-east-1";
    }
    if (m_scheme.has_value())
    {
        switch (*m_scheme)
        {
        case Scheme::HTTP:
            config.scheme = Aws::Http::Scheme::HTTP;
            break;
        case Scheme::HTTPS:
            config.scheme = Aws::Http::Scheme::HTTPS;
            break;
            break;
        }
    }

    config.connectTimeoutMs = 5000;
    config.requestTimeoutMs = 15000;

    auto aws_credentials = [&]() -> Aws::Auth::AWSCredentials {
        if (m_sessionToken.has_value())
        {
            return {m_accessKeyId, m_secretKey, *m_sessionToken};
        }
        else
        {
            return {m_accessKeyId, m_secretKey};
        }
    }();

    Aws::S3::S3Client s3_client(
        aws_credentials,
        config,
        Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
        false);

    return ExternalBlockStorage{std::make_unique<ExternalBlockStorageAws>(
        std::move(s3_client), std::move(m_bucketName))};
}

auto AwsBuilder::build() -> ExternalBlockStorage
{
    return *this;
}

} // namespace openPMD::internal
