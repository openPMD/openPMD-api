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
