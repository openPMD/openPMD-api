#pragma once

#include "openPMD/config.hpp"

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>

namespace openPMD
{
class ExternalBlockStorage;
}

namespace openPMD::internal
{
struct AwsBuilder
{
    AwsBuilder(
        std::string bucketName, std::string accessKeyId, std::string secretKey);

    enum class Scheme : uint8_t
    {
        HTTP,
        HTTPS
    };
    std::string m_bucketName;
    std::string m_accessKeyId;
    std::string m_secretKey;
    std::optional<std::string> m_sessionToken;
    std::initializer_list<std::string> m_credentials;
    std::optional<std::string> m_endpointOverride;
    std::optional<std::string> m_region;
    std::optional<Scheme> m_scheme;
    std::optional<bool> m_verifySSL;
    std::optional<bool> m_useAsyncIO;

    auto setBucketName(std::string bucketName) -> AwsBuilder &;
    auto setCredentials(std::string accessKeyId, std::string secretKey)
        -> AwsBuilder &;
    auto setSessionToken(std::string sessionToken) -> AwsBuilder &;
    auto setEndpointOverride(std::string endpoint) -> AwsBuilder &;
    auto setRegion(std::string regionName) -> AwsBuilder &;
    auto setScheme(Scheme s) -> AwsBuilder &;
    auto setVerifySSL(bool verify) -> AwsBuilder &;
    auto setAsyncIO(bool useAsyncIO) -> AwsBuilder &;

    operator ::openPMD::ExternalBlockStorage();
    auto build() -> ::openPMD::ExternalBlockStorage;
};
} // namespace openPMD::internal
