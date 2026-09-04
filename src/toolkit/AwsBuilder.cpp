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
#include "openPMD/config.hpp"

#include "openPMD/toolkit/AwsBuilder.hpp"

#include "openPMD/toolkit/Aws.hpp"
#include "openPMD/toolkit/ExternalBlockStorage.hpp"

#if openPMD_HAVE_AWS
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/signer/AWSAuthV4Signer.h>
#include <aws/core/http/Scheme.h>
#endif

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

auto AwsBuilder::setVerifySSL(bool verify) -> AwsBuilder &
{
    m_verifySSL = verify;
    return *this;
}

auto AwsBuilder::setAsyncIO(bool useAsyncIO) -> AwsBuilder &
{
    m_useAsyncIO = useAsyncIO;
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
#if openPMD_HAVE_AWS
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

    if (m_verifySSL.has_value())
    {
        config.verifySSL = *m_verifySSL;
    }

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
        std::move(s3_client),
        std::move(m_bucketName),
        std::move(m_endpointOverride),
        m_useAsyncIO.value_or(true))};
#else
    throw std::runtime_error(
        "Method not available: openPMD-api has been built without support for "
        "AWS.");
#endif
}

auto AwsBuilder::build() -> ExternalBlockStorage
{
    return *this;
}

} // namespace openPMD::internal
