#pragma once

#include <cstdint>

namespace openPMD::internal
{
/** Which API a load/store chunk operation originates from.
 *
 * This governs the flushing behavior of an operation and interacts with the
 * synchronous flush option (Series option ``"flush_immediately"`` /
 * ``OPENPMD_FLUSH_IMMEDIATELY``):
 *
 * - The *chaining* API (``RecordComponent::prepareLoadStore()``) performs an
 *   automatic flush as soon as the returned ``DeferredComputation`` object is
 *   evaluated. Hence, its operations are safe to run without an explicit
 *   ``flush()`` and the sync-flush option has no effect on them.
 * - The *legacy* API (``RecordComponent::storeChunk()`` /
 *   ``loadChunk()`` and friends) only enqueues operations which are performed
 *   at flush points. With the sync-flush option enabled, each such operation
 *   becomes its own flush point, i.e. it is flushed immediately.
 */
enum class LS_API : std::uint8_t
{
    legacy,
    chaining
};
} // namespace openPMD::internal
