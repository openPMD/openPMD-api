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

#include "openPMD/Dataset.hpp"
#include "openPMD/auxiliary/Memory.hpp"
#include "openPMD/toolkit/AwsBuilder.hpp"
#include "openPMD/toolkit/StdioBuilder.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
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
    put(std::string const &identifier, auxiliary::WriteBuffer data, size_t len)
        -> std::string = 0;
    virtual void
    get(std::string const &external_ref,
        std::shared_ptr<void> data,
        size_t len) = 0;
    [[nodiscard]] virtual auto externalStorageLocation() const
        -> nlohmann::json = 0;

    virtual void syncMandatoryOperations();
    virtual void syncAllOperations();

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
        auxiliary::WriteBuffer data) -> std::string;

    template <typename DatatypeHandling, typename T>
    void read(
        std::string const &identifier,
        nlohmann::json const &fullJsonDataset,
        nlohmann::json::json_pointer const &path,
        std::shared_ptr<void> &data);

    template <typename DatatypeHandling, typename T>
    void read(
        Offset const &blockOffset,
        Extent const &blockExtent,
        nlohmann::json const &fullJsonDataset,
        nlohmann::json::json_pointer const &path,
        std::shared_ptr<void> &data);

    void syncMandatoryOperations();
    void syncAllOperations();

    [[nodiscard]] auto externalStorageLocation() const -> nlohmann::json;

    static void sanitizeString(std::string &s);
};

// Implementations

} // namespace openPMD
