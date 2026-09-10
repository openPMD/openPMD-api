#pragma once

#include <cstdint>

namespace openPMD::internal
{
// Load Store API
enum class LS_API : std::uint8_t
{
    legacy,
    chaining
};
} // namespace openPMD::internal
