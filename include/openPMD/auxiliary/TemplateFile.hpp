#pragma once

#include "openPMD/Series.hpp"

namespace openPMD::auxiliary
{
// @todo replace uint64_t with proper type after merging #1285
Series &initializeFromTemplate(
    Series &initializeMe, Series const &fromTemplate, uint64_t iteration);
} // namespace openPMD::auxiliary
