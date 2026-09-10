#pragma once

#include <cstdint>

namespace openPMD::internal
{
enum class API : std::uint8_t
{
    legacy,
    chaining
};
}
