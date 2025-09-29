
#include "openPMD/toolkit/ExternalBlockStorage.hpp"

#include "openPMD/DatatypeMacros.hpp"
#include "openPMD/IO/JSON/JSONIOHandlerImpl.hpp"
#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/toolkit/Aws.hpp"
#include "openPMD/toolkit/Stdio.hpp"

#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/auth/signer/AWSAuthV4Signer.h>
#include <aws/core/http/Scheme.h>
#include <aws/core/utils/memory/stl/AWSStreamFwd.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <cstdio>
#include <istream>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <utility>

namespace
{
auto concat_filepath(std::string const &s1, std::string const &s2)
    -> std::string
{
    if (s1.empty())
    {
        return s2;
    }
    if (s2.empty())
    {
        return s1;
    }
    bool ends_with_slash =
        *s1.crbegin() == openPMD::auxiliary::directory_separator;
    bool starts_with_slash =
        *s2.cbegin() == openPMD::auxiliary::directory_separator;

    if (ends_with_slash ^ starts_with_slash)
    {
        return s1 + s2;
    }
    else if (ends_with_slash && starts_with_slash)
    {
        return s1 + (s2.c_str() + 1);
    }
    else
    {
        return s1 + openPMD::auxiliary::directory_separator + s2;
    }
}
} // namespace

namespace openPMD::internal
{
ExternalBlockStorageBackend::~ExternalBlockStorageBackend() = default;

ExternalBlockStorageStdio::ExternalBlockStorageStdio(
    std::string directory, std::string openMode)
    : m_directory(std::move(directory)), m_openMode(std::move(openMode))
{
    if (m_directory.empty())
    {
        throw std::invalid_argument(
            "ExternalBlockStorageStdio: directory cannot be empty");
    }

    // Ensure the directory exists and is writable
    if (!auxiliary::create_directories(m_directory))
    {
        throw std::runtime_error(
            "ExternalBlockStorageStdio: failed to create or access "
            "directory: " +
            m_directory);
    }
}

ExternalBlockStorageStdio::~ExternalBlockStorageStdio() = default;

auto ExternalBlockStorageStdio::put(
    std::string const &identifier, void const *data, size_t len) -> std::string
{
    auto sanitized = identifier + ".dat";
    ExternalBlockStorage::sanitizeString(sanitized);
    std::string filepath = concat_filepath(m_directory, sanitized);

    if (len == 0)
    {
        return filepath;
    }

    FILE *file = std::fopen(filepath.c_str(), "wb");
    if (!file)
    {
        throw std::runtime_error(
            "ExternalBlockStorageStdio: failed to open file for writing: " +
            filepath);
    }

    size_t written = std::fwrite(data, 1, len, file);
    if (written != len)
    {
        throw std::runtime_error(
            "ExternalBlockStorageStdio: failed to write full data to file: " +
            filepath);
    }

    if (std::fclose(file) != 0)
    {
        throw std::runtime_error(
            "ExternalBlockStorageStdio: failed to close file after writing: " +
            filepath);
    }

    return filepath;
}

auto StdioBuilder::setDirectory(std::string directory) -> StdioBuilder &
{
    m_directory = std::move(directory);
    return *this;
}
auto StdioBuilder::setOpenMode(std::string openMode) -> StdioBuilder &
{
    m_openMode = std::move(openMode);
    return *this;
}

ExternalBlockStorageAws::ExternalBlockStorageAws(
    Aws::S3::S3Client client, std::string bucketName)
    : m_client{std::move(client)}, m_bucketName(std::move(bucketName))
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

namespace
{
    struct membuf : std::streambuf
    {
        membuf(char const *base, std::size_t size)
        {
            // hm hm
            auto p = const_cast<char *>(base);
            this->setg(p, p, p + size); // setup get area
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

auto ExternalBlockStorageAws::put(
    std::string const &identifier, void const *data, size_t len) -> std::string
{
    auto sanitized = identifier;
    ExternalBlockStorage::sanitizeString(sanitized);

    Aws::S3::Model::PutObjectRequest put_request;
    put_request.SetBucket(m_bucketName);
    put_request.SetKey(sanitized);

    auto input_data = Aws::MakeShared<imemstream>(
        "PutObjectInputStream", reinterpret_cast<char const *>(data), len);
    std::static_pointer_cast<Aws::IOStream>(input_data);

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

StdioBuilder::operator ExternalBlockStorage()
{
    return ExternalBlockStorage{std::make_unique<ExternalBlockStorageStdio>(
        std::move(m_directory), std::move(m_openMode).value_or("wb"))};
}

auto StdioBuilder::build() -> ExternalBlockStorage
{
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

    // default timeout
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

    // Create the S3 client
    Aws::S3::S3Client s3_client(
        aws_credentials,
        config,
        Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
        false);

    // Create the AWS storage backend
    return ExternalBlockStorage{std::make_unique<ExternalBlockStorageAws>(
        std::move(s3_client), std::move(m_bucketName))};
}
} // namespace openPMD::internal

namespace openPMD
{
ExternalBlockStorage::ExternalBlockStorage() = default;
ExternalBlockStorage::ExternalBlockStorage(
    std::unique_ptr<internal::ExternalBlockStorageBackend> worker)
    : m_worker(std::move(worker))
{}

auto ExternalBlockStorage::makeStdioSession(std::string directory)
    -> internal::StdioBuilder
{
    return internal::StdioBuilder{std::move(directory)};
}

auto ExternalBlockStorage::makeAwsSession(
    std::string bucketName, std::string accessKeyId, std::string secretKey)
    -> internal::AwsBuilder
{
    return internal::AwsBuilder(
        std::move(bucketName), std::move(accessKeyId), std::move(secretKey));
}

template <typename DatatypeHandling, typename T>
auto ExternalBlockStorage::store(
    Extent globalExtent,
    Offset blockOffset,
    Extent blockExtent,
    nlohmann::json &fullJsonDataset,
    nlohmann::json::json_pointer const &path,
    T const *data) -> std::string
{
    // JSON Identifier: running counter.
    // Do not use an array to avoid reindexing upon deletion.

    // Filesystem Identifier: JSON path + running counter.

    // For each externally handled data block, store:
    // 1. Filesystem identifier
    // 2. Offset, Extent
    auto &dataset = fullJsonDataset[path];

    // running_index denotes the last *used* block index in the dataset
    using running_index_t = uint64_t;
    running_index_t running_index = [&]() -> running_index_t {
        if (auto it = dataset.find("_running_index"); it != dataset.end())
        {
            auto res = it->get<running_index_t>();
            ++res;
            *it = res;
            return res;
        }
        else
        {
            dataset["_running_index"] = 0;
            return 0;
        }
    }();

    constexpr size_t padding = 6;
    std::string index_as_str = [running_index]() {
        auto res = std::to_string(running_index);
        auto size = res.size();
        if (size >= padding)
        {
            return res;
        }
        std::stringstream padded;
        for (size_t i = 0; i < padding - size; ++i)
        {
            padded << '0';
        }
        padded << res;
        return padded.str();
    }();

    if (dataset.contains(index_as_str))
    {
        throw std::runtime_error(
            "Inconsistent state: Index " + index_as_str + " already in use.");
    }

    auto check_metadata = [&dataset](char const *key, auto const &value) {
        using value_t =
            std::remove_reference_t<std::remove_cv_t<decltype(value)>>;
        if (auto it = dataset.find(key); it != dataset.end())
        {
            auto const &stored_value = it->get<value_t>();
            if (stored_value != value)
            {
                throw std::runtime_error(
                    "Inconsistent chunk storage in key " + std::string(key) +
                    ".");
            }
        }
        else
        {
            dataset[key] = value;
        }
    };
    if (!DatatypeHandling::template encodeDatatype<T>(dataset))
    {
        throw std::runtime_error("Inconsistent chunk storage in datatype.");
    }
    check_metadata("_byte_width", sizeof(T));
    check_metadata("_extent", globalExtent);

    auto &block = dataset[index_as_str];
    block["offset"] = blockOffset;
    block["extent"] = blockExtent;
    std::stringstream filesystem_identifier;
    filesystem_identifier << path.to_string() << "--" << index_as_str;
    auto escaped_filesystem_identifier = m_worker->put(
        filesystem_identifier.str(),
        data,
        std::accumulate(
            blockExtent.begin(),
            blockExtent.end(),
            sizeof(T),
            [](size_t left, size_t right) { return left * right; }));
    block["external_ref"] = escaped_filesystem_identifier;
    return index_as_str;
}

void ExternalBlockStorage::sanitizeString(std::string &s)
{
    // Replace invalid characters with underscore
    for (char &c : s)
    {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' ||
            c == '"' || c == '<' || c == '>' || c == '|' || c == '\n' ||
            c == '\r' || c == '\t' || c == '\0' || c == ' ')
        {
            c = '_';
        }
    }
}

#define OPENPMD_INSTANTIATE_DATATYPEHANDLING(datatypehandling, type)           \
    template auto ExternalBlockStorage::store<datatypehandling, type>(         \
        Extent globalExtent,                                                   \
        Offset blockOffset,                                                    \
        Extent blockExtent,                                                    \
        nlohmann::json & fullJsonDataset,                                      \
        nlohmann::json::json_pointer const &path,                              \
        type const *data) -> std::string;
#define OPENPMD_INSTANTIATE(type)                                              \
    OPENPMD_INSTANTIATE_DATATYPEHANDLING(internal::JsonDatatypeHandling, type)
OPENPMD_FOREACH_DATASET_DATATYPE(OPENPMD_INSTANTIATE)
#undef OPENPMD_INSTANTIATE
} // namespace openPMD
