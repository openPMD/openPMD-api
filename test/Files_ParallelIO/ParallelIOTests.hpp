/* Copyright 2025 Franz Poeschel
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
#include <openPMD/openPMD.hpp>

#if openPMD_HAVE_MPI

using namespace openPMD;

struct BackendSelection
{
    std::string backendName;
    std::string extension;

    [[nodiscard]] inline std::string jsonBaseConfig() const
    {
        return R"({"backend": ")" + backendName + "\"}";
    }
};

inline std::vector<BackendSelection> testedBackends()
{
    auto variants = getVariants();
    std::map<std::string, std::string> extensions{
        {"adios2", "bp"}, {"hdf5", "h5"}};
    std::vector<BackendSelection> res;
    for (auto const &pair : variants)
    {
        if (pair.second)
        {
            auto lookup = extensions.find(pair.first);
            if (lookup != extensions.end())
            {
                std::string extension = lookup->second;
                res.push_back({pair.first, std::move(extension)});
            }
        }
    }
    return res;
}

inline std::vector<std::string> getBackends()
{
    // first component: backend file ending
    // second component: whether to test 128 bit values
    std::vector<std::string> res;
#if openPMD_HAVE_ADIOS2
    res.emplace_back("bp");
#endif
#if openPMD_HAVE_HDF5
    res.emplace_back("h5");
#endif
    return res;
}

inline auto const backends = getBackends();

inline std::vector<std::string> testedFileExtensions()
{
    auto allExtensions = getFileExtensions();
    auto newEnd = std::remove_if(
        allExtensions.begin(), allExtensions.end(), [](std::string const &ext) {
            // sst and ssc need a receiver for testing
            // bp4 is already tested via bp
            return ext == "sst" || ext == "ssc" || ext == "bp4" ||
                ext == "toml" || ext == "json";
        });
    return {allExtensions.begin(), newEnd};
}

namespace read_variablebased_randomaccess
{
auto read_variablebased_randomaccess() -> void;
}

namespace iterate_nonstreaming_series
{
auto iterate_nonstreaming_series() -> void;
}

namespace bug_1655_bp5_writer_hangup
{
auto bug_1655_bp5_writer_hangup() -> void;
}

#endif
