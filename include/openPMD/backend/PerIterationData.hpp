#pragma once

#include "openPMD/ChunkInfo.hpp"
#include "openPMD/Streaming.hpp"
#include "openPMD/backend/Attributable.hpp"

#include <variant>

namespace openPMD::internal
{
struct NoSourceSpecified
{};
struct SourceSpecifiedViaJSON
{
    std::string value;
};
struct SourceSpecifiedManually
{
    std::string value;
};

struct RankTableData
{
    Attributable m_attributable;
    std::variant<
        NoSourceSpecified,
        SourceSpecifiedViaJSON,
        SourceSpecifiedManually>
        m_rankTableSource;
    std::optional<chunk_assignment::RankMeta> m_bufferedRead;
};

/*
 * This stores data items that are:
 *
 * 1. global in group and variable encodings
 * 2. per-iteration in file encoding
 *
 * The struct is stored as part of the Series and as part of each Iteration.
 * Access must be distinguished by iteration encoding.
 */
struct PerIterationData
{
    /**
     *  Whether a step is currently active for this iteration.
     * Used for group-based iteration layout, see SeriesData.hpp for
     * iteration-based layout.
     * Access via stepStatus() method to automatically select the correct
     * one among both flags.
     */
    StepStatus m_stepStatus = StepStatus::NoStep;
    Attributable m_rankTableAttributable;
};
} // namespace openPMD::internal
