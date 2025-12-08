#pragma once

#include "openPMD/Dataset.hpp"
#include "openPMD/toolkit/AwsBuilder.hpp"
#include "openPMD/toolkit/StdioBuilder.hpp"

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
    virtual void
    get(std::string const &external_ref, void *data, size_t len) = 0;
    [[nodiscard]] virtual auto externalStorageLocation() const
        -> nlohmann::json = 0;

    virtual ~ExternalBlockStorageBackend();
};
} // namespace openPMD::internal

namespace openPMD
{
// used nowhere, just shows the signatures
// TODO: replace this with a concept upon switching to C++20
struct DatatypeHandling_Interface
{
    /*
     * Returns false if the same JSON location was previously encoded as
     * another datatype.
     */
    template <typename T>
    static auto encodeDatatype(nlohmann::json &) -> bool;

    /*
     * Returns false if the encoded datatype does not match T_required
     * or if no datatype has been encoded.
     */
    template <typename T_required>
    static auto checkDatatype(nlohmann::json const &j) -> bool;

    /*
     * Returns false if no encoded datatype could be found
     */
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
    static auto makeAwsSession(
        std::string bucketName, std::string accessKeyId, std::string secretKey)
        -> internal::AwsBuilder;

    // returns created JSON key
    template <typename DatatypeHandling, typename T>
    auto store(
        Extent const &globalExtent,
        Offset const &blockOffset,
        Extent const &blockExtent,
        nlohmann::json &fullJsonDataset,
        nlohmann::json::json_pointer const &path,
        std::optional<std::string> infix, // e.g. for distinguishing MPI ranks
        T const *data) -> std::string;

    template <typename DatatypeHandling, typename T>
    void read(
        std::string const &identifier,
        nlohmann::json const &fullJsonDataset,
        nlohmann::json::json_pointer const &path,
        T *data);

    template <typename DatatypeHandling, typename T>
    void read(
        Offset const &blockOffset,
        Extent const &blockExtent,
        nlohmann::json const &fullJsonDataset,
        nlohmann::json::json_pointer const &path,
        T *data);

    [[nodiscard]] auto externalStorageLocation() const -> nlohmann::json;

    static void sanitizeString(std::string &s);
};

// Implementations

} // namespace openPMD
