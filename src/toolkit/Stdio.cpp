#include "openPMD/toolkit/Stdio.hpp"

#include "openPMD/auxiliary/Filesystem.hpp"

#include <cstdio>
#include <stdexcept>

namespace openPMD::internal
{
ExternalBlockStorageStdio::ExternalBlockStorageStdio(
    std::string directory, std::string openMode)
    : m_directory(std::move(directory)), m_openMode(std::move(openMode))
{
    if (m_directory.empty())
    {
        throw std::invalid_argument(
            "ExternalBlockStorageStdio: directory cannot be empty");
    }

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
    std::string filepath = m_directory + "/" + sanitized;

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
} // namespace openPMD::internal
