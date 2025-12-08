#include "openPMD/toolkit/Stdio.hpp"

#include "openPMD/auxiliary/Filesystem.hpp"

#include <cstdio>
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
    std::string filepath = concat_filepath(m_directory, sanitized);

    if (len == 0)
    {
        return filepath;
    }

    FILE *file = std::fopen(filepath.c_str(), m_openMode.c_str());
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

    return sanitized;
}

void ExternalBlockStorageStdio::get(
    std::string const &external_ref, void *data, size_t len)
{
    if (len == 0)
    {
        return;
    }

    std::string filepath = concat_filepath(m_directory, external_ref);

    FILE *file = std::fopen(filepath.c_str(), "rb");
    if (!file)
    {
        throw std::runtime_error(
            "ExternalBlockStorageStdio: failed to open file for reading: " +
            filepath);
    }

    size_t read = std::fread(data, 1, len, file);
    if (read != len)
    {
        std::fclose(file);
        throw std::runtime_error(
            "ExternalBlockStorageStdio: failed to read full data from file: " +
            filepath);
    }

    if (std::fclose(file) != 0)
    {
        throw std::runtime_error(
            "ExternalBlockStorageStdio: failed to close file after reading: " +
            filepath);
    }
}

[[nodiscard]] auto ExternalBlockStorageStdio::externalStorageLocation() const
    -> nlohmann::json
{
    nlohmann::json j;
    j["provider"] = "stdio";
    j["directory"] = m_directory;
    j["open_mode"] = m_openMode;
    return j;
}
} // namespace openPMD::internal
