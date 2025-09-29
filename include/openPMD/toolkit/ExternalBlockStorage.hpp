#pragma once

#include "openPMD/Dataset.hpp"

#include <initializer_list>
#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace openPMD
{
class ExternalBlockStorage;
}

namespace openPMD::internal
{
struct ExternalBlockStorageBackend
{
    virtual auto
    put(std::string const &identifier, void const *data, size_t len)
        -> std::string = 0;
    virtual ~ExternalBlockStorageBackend();
};

struct StdioBuilder
{
    std::string m_directory;
    std::optional<std::string> m_openMode = std::nullopt;

    auto setDirectory(std::string directory) -> StdioBuilder &;
    auto setOpenMode(std::string openMode) -> StdioBuilder &;

    operator ExternalBlockStorage();
    auto build() -> ExternalBlockStorage;
};

struct AwsBuilder
{
    struct init_credentials_tag_t
    {};
    static constexpr init_credentials_tag_t init_credentials_tag = {};

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

    auto setBucketName(std::string bucketName) -> AwsBuilder &;
    auto setCredentials(std::string accessKeyId, std::string secretKey)
        -> AwsBuilder &;
    auto setSessionToken(std::string sessionToken) -> AwsBuilder &;
    auto setEndpointOverride(std::string endpoint) -> AwsBuilder &;
    auto setRegion(std::string regionName) -> AwsBuilder &;
    auto setScheme(Scheme s) -> AwsBuilder &;

    operator ExternalBlockStorage();
    auto build() -> ExternalBlockStorage;
};
} // namespace openPMD::internal

namespace openPMD
{
// used nowhere, just shows the signatures
// TODO: replace this with a concept upon switching to C++20
struct DatatypeHandling_Interface
{
    template <typename T>
    static auto encodeDatatype(nlohmann::json &) -> bool;

    template <typename Functor, typename... Args>
    static auto decodeDatatype(nlohmann::json const &j, Args &&...args) -> bool;
};

class ExternalBlockStorage
{
private:
    std::unique_ptr<internal::ExternalBlockStorageBackend> m_worker;
    ExternalBlockStorage(
        std::unique_ptr<internal::ExternalBlockStorageBackend>);

    friend struct internal::StdioBuilder;
    friend struct internal::AwsBuilder;

public:
    explicit ExternalBlockStorage();

    static auto makeStdioSession(std::string directory)
        -> internal::StdioBuilder;
    template <typename... Args>
    static auto makeAwsSession(
        std::string bucketName, std::string accessKeyId, std::string secretKey)
        -> internal::AwsBuilder;

    // returns created JSON key
    template <typename DatatypeHandling, typename T>
    auto store(
        Extent globalExtent,
        Offset blockOffset,
        Extent blockExtent,
        nlohmann::json &fullJsonDataset,
        nlohmann::json::json_pointer const &path,
        T const *data) -> std::string;

    static void sanitizeString(std::string &s);
};

// Implementations

} // namespace openPMD
