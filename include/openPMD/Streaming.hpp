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
/**
 * In step-based mode (i.e. when using the Streaming API),
 * indicate whether there are further steps to read or not
 * (i.e. the stream is not over or it is).
 *
 */
enum class AdvanceStatus : unsigned char
{
    OK, /* stream goes on */
    OVER /* stream is over */
};

/**
 * In step-based mode (i.e. when using the Streaming API),
 * stepping/advancing through the Series is performed in terms
 * of interleaving begin- and end-step calls.
 * Distinguish both kinds by using this enum.
 *
 */
enum class AdvanceMode : unsigned char
{
    BEGINSTEP,
    ENDSTEP
};

/**
 * Used in step-based mode (i.e. when using the Streaming API)
 * to determine whether a step is currently active or not.
 *
 */
enum class StepStatus : unsigned char
{
    DuringStep, /* step is currently active */
    NoStep /* no step is currently active */
};
} // namespace openPMD
