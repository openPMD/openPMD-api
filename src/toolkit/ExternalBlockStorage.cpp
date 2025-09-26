
#include "openPMD/toolkit/ExternalBlockStorage.hpp"

#include "openPMD/auxiliary/Filesystem.hpp"
#include "openPMD/toolkit/ExternalBlockStorage_internal.hpp"

#include <cstdio>
#include <memory>
#include <stdexcept>

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
    std::string filepath = concat_filepath(m_directory, identifier);
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

StdioBuilder::operator ExternalBlockStorage()
{
    return ExternalBlockStorage{std::make_unique<ExternalBlockStorageStdio>(
        std::move(m_directory), std::move(m_openMode).value_or("wb"))};
}

auto StdioBuilder::build() -> ExternalBlockStorage
{
    return *this;
}
} // namespace openPMD::internal

namespace openPMD
{
auto ExternalBlockStorage::makeStdioSession(std::string directory)
    -> internal::StdioBuilder
{
    return internal::StdioBuilder{std::move(directory)};
}

ExternalBlockStorage::ExternalBlockStorage(
    std::unique_ptr<internal::ExternalBlockStorageBackend> worker)
    : m_worker(std::move(worker))
{}
} // namespace openPMD
