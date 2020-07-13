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
enum class AdvanceStatus : unsigned char
{
    OK,
    OVER
};

enum class AdvanceMode : unsigned char
{
    BEGINSTEP,
    ENDSTEP
};

enum class StepStatus : unsigned char
{
    DuringStep,
    NoStep
};
} // namespace openPMD