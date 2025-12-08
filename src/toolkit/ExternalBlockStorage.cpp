#include "openPMD/toolkit/ExternalBlockStorage.hpp"

#include "openPMD/DatatypeMacros.hpp"
#include "openPMD/IO/JSON/JSONIOHandlerImpl.hpp"
#include "openPMD/auxiliary/StringManip.hpp"

#include <nlohmann/json.hpp>

#include <numeric>
#include <sstream>
#include <stdexcept>

namespace openPMD::internal
{
ExternalBlockStorageBackend::~ExternalBlockStorageBackend() = default;
}

namespace openPMD
{

namespace
{
    auto flat_extent(Extent const &e) -> size_t
    {
        return std::accumulate(
            e.begin(), e.end(), 1, [](size_t left, size_t right) {
                return left * right;
            });
    }

    template <typename T>
    void read_impl(
        internal::ExternalBlockStorageBackend *backend,
        nlohmann::json const &external_block,
        T *data,
        size_t len)
    {
        auto const &external_ref =
            external_block.at("external_ref").get<std::string>();
        backend->get(external_ref, data, sizeof(T) * len);
    }
} // namespace

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
    Extent const &globalExtent,
    Offset const &blockOffset,
    Extent const &blockExtent,
    nlohmann::json &fullJsonDataset,
    nlohmann::json::json_pointer const &path,
    std::optional<std::string> infix,
    T const *data) -> std::string
{
    auto &dataset = fullJsonDataset[path];

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
    check_metadata("byte_width", sizeof(T));
    check_metadata("extent", globalExtent);

    auto &block = dataset["external_blocks"][index_as_str];
    block["offset"] = blockOffset;
    block["extent"] = blockExtent;
    std::stringstream filesystem_identifier;
    filesystem_identifier << path.to_string();
    if (infix.has_value())
    {
        filesystem_identifier << "--" << *infix;
    }
    filesystem_identifier << "--" << index_as_str;
    auto escaped_filesystem_identifier = m_worker->put(
        filesystem_identifier.str(),
        data,
        sizeof(T) * flat_extent(blockExtent));
    block["external_ref"] = escaped_filesystem_identifier;
    return index_as_str;
}

template <typename DatatypeHandling, typename T>
void ExternalBlockStorage::read(
    [[maybe_unused]] std::string const &identifier,
    [[maybe_unused]] nlohmann::json const &fullJsonDataset,
    [[maybe_unused]] nlohmann::json::json_pointer const &path,
    [[maybe_unused]] T *data)
{}

template <typename DatatypeHandling, typename T>
void ExternalBlockStorage::read(
    Offset const &blockOffset,
    Extent const &blockExtent,
    nlohmann::json const &fullJsonDataset,
    nlohmann::json::json_pointer const &path,
    T *data)
{
    auto &dataset = fullJsonDataset[path];
    if (!DatatypeHandling::template checkDatatype<T>(dataset))
    {
        throw std::runtime_error("Inconsistent chunk storage in datatype.");
    }
    auto external_blocks = dataset.at("external_blocks");
    bool found_a_precise_match = false;
    for (auto it = external_blocks.begin(); it != external_blocks.end(); ++it)
    {
        auto const &block = it.value();
        try
        {
            auto const &o = block.at("offset").get<Offset>();
            auto const &e = block.at("extent").get<Extent>();
            // Look only for exact matches for now
            if (o != blockOffset || e != blockExtent)
            {
                continue;
            }
            found_a_precise_match = true;
            read_impl(m_worker.get(), block, data, flat_extent(blockExtent));
            break;
        }
        catch (nlohmann::json::exception const &e)
        {
            std::cerr << "[ExternalBlockStorage::read] Could not parse block '"
                      << it.key() << "'. Original error was:\n"
                      << e.what();
        }
    }
    if (!found_a_precise_match)
    {
        throw std::runtime_error(
            "[ExternalBlockStorage::read] Unable to find a precise match for "
            "offset " +
            auxiliary::vec_as_string(blockOffset) + " and extent " +
            auxiliary::vec_as_string(blockExtent));
    }
}

[[nodiscard]] auto ExternalBlockStorage::externalStorageLocation() const
    -> nlohmann::json
{
    return m_worker->externalStorageLocation();
}

void ExternalBlockStorage::sanitizeString(std::string &s)
{
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
        Extent const &globalExtent,                                            \
        Offset const &blockOffset,                                             \
        Extent const &blockExtent,                                             \
        nlohmann::json &fullJsonDataset,                                       \
        nlohmann::json::json_pointer const &path,                              \
        std::optional<std::string> infix,                                      \
        type const *data) -> std::string;                                      \
    template void ExternalBlockStorage::read<datatypehandling, type>(          \
        std::string const &identifier,                                         \
        nlohmann::json const &fullJsonDataset,                                 \
        nlohmann::json::json_pointer const &path,                              \
        type *data);                                                           \
    template void ExternalBlockStorage::read<datatypehandling, type>(          \
        Offset const &blockOffset,                                             \
        Extent const &blockExtent,                                             \
        nlohmann::json const &fullJsonDataset,                                 \
        nlohmann::json::json_pointer const &path,                              \
        type *data);
#define OPENPMD_INSTANTIATE(type)                                              \
    OPENPMD_INSTANTIATE_DATATYPEHANDLING(internal::JsonDatatypeHandling, type)
OPENPMD_FOREACH_DATASET_DATATYPE(OPENPMD_INSTANTIATE)
#undef OPENPMD_INSTANTIATE

} // namespace openPMD
