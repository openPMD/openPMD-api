#pragma once

#include <list>
#include <string>
#include <utility>
#include <vector>

#include "openPMD/Dataset.hpp"
#include "openPMD/benchmark/mpi/BlockSlicer.hpp"
#include <unordered_map>

namespace openPMD
{
enum class AdvanceStatus
{
    OK,
    OVER
};

enum class AdvanceMode
{
    AUTO, // according to accesstype
    READ,
    WRITE
};
} // namespace openPMD