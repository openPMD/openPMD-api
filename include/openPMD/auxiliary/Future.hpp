#pragma once

#include <functional>
#include <type_traits>
#include <variant>

namespace openPMD::auxiliary::detail
{
/** Internal helper for deferred computation - executes task once */
template <typename T>
struct OneTimeTask
{
    using task_type = std::function<T()>;
    // Helper struct so we get auto-generated move constructor / assignment
    // operator, but can still override constructors outside
    struct Members
    {
        task_type m_task;
        bool m_task_valid = true;
    };
    Members members;

    static constexpr bool noexcept_move =
        std::is_move_constructible_v<Members> &&
        std::is_move_assignable_v<Members>;

    explicit OneTimeTask();
    OneTimeTask(task_type);

    OneTimeTask(OneTimeTask &&) noexcept(noexcept_move);
    OneTimeTask(OneTimeTask const &) = delete;

    auto operator=(OneTimeTask &&) noexcept(noexcept_move) -> OneTimeTask &;
    auto operator=(OneTimeTask const &) -> OneTimeTask & = delete;

    auto operator()() -> T;

    [[nodiscard]] auto valid() const -> bool;
};

/** Internal helper for cached value storage. Used when the API requires
 * creation of a DeferredComputation object, but there is not actually a
 * computation to run. */
template <typename T>
struct CachedValue
{
    T val;
};
template <>
struct CachedValue<void>
{
    // this is silly
};
} // namespace openPMD::auxiliary::detail

namespace openPMD::auxiliary
{
/** A computation that is deferred until explicitly invoked.
 *
 * This class wraps a callable, allowing lazy evaluation.
 * The computation is performed once on first invocation, repeated invocation is
 * an error. Check if the computation is still valid by calling valid().
 *
 * Note: Some API operations may construct a DeferredComputation without any
 * actual computation, instead emplacing a cached value. This is treated
 * transparently to the user. In this case however, the object will not turn
 * invalid upon invocation.
 *
 * @tparam T The return type of the computation
 */
template <typename T>
class DeferredComputation
{
    using task_type = std::function<T()>;
    using cached_type = std::conditional_t<
        std::is_void_v<T>,
        // just something that is not void
        detail::CachedValue<void>,
        T>;
    std::variant<detail::OneTimeTask<T>, detail::CachedValue<T>> m_task;

public:
    static constexpr bool noexcept_move =
        std::is_move_constructible_v<detail::OneTimeTask<T>> &&
        std::is_move_assignable_v<detail::OneTimeTask<T>> &&
        std::is_move_constructible_v<detail::CachedValue<T>> &&
        std::is_move_assignable_v<detail::CachedValue<T>>;
    /** Construct from a callable
     *
     * @param task The callable to execute
     */
    DeferredComputation(task_type task);
    /** Construct from a cached value
     *
     * @param val The pre-computed value
     */
    DeferredComputation(cached_type val);

    explicit DeferredComputation();

    DeferredComputation(DeferredComputation &&) noexcept(noexcept_move);
    DeferredComputation(DeferredComputation const &) = delete;

    /** Move-assign, discarding any still-pending task of the target.
     *
     * A task already held by the target is not executed; a warning is printed
     * to standard error instead. For handles returned by openPMD's data
     * operations this means that the automatic flush is skipped, while the
     * data operation itself has already been enqueued.
     */
    auto operator=(DeferredComputation &&) noexcept(noexcept_move)
        -> DeferredComputation &;
    auto operator=(DeferredComputation const &)
        -> DeferredComputation & = delete;

    /** Destroy the computation without executing a still-pending task.
     *
     * Destroying a still-valid handle does not run its callable and therefore
     * does not trigger the automatic flush of the underlying Series. If the
     * handle came from ConfigureLoadStore::store() / load(), the data operation
     * itself has already been enqueued by that call; only the flush is skipped,
     * so the user must ensure that the Series is flushed at a later point. If
     * the handle is still valid, a warning is printed to standard error.
     */
    ~DeferredComputation();

    /** Get the result of the computation
     *
     * Invoking the computation executes the stored callable. For handles
     * returned by openPMD's store() / load(), this triggers the automatic
     * flush of the underlying Series (unless disabled via
     * ConfigureLoadStore::unsafeNoAutomaticFlush()); the data operation itself
     * was already enqueued when store() / load() was called. Since
     * Series::flush() is MPI-collective, all ranks of a parallel Series must
     * invoke their deferred computations in a consistent order. The handles
     * returned by openPMD's store() / load() operations are marked
     * [[nodiscard]] so that they cannot be dropped unnoticed. The flush itself
     * is skipped if the pertaining RecordComponent has already been flushed by
     * another component in the meantime (tracked via per-component flush
     * counters), so a given invocation does not necessarily flush.
     *
     * @return The result of the computation
     */
    auto get() -> T;
    /** Invoke the computation
     *
     * Alias for get(). See get() for the collective semantics of the automatic
     * flush.
     *
     * @return The result of the computation
     */
    auto operator()() -> T;

    /** Discard the computation without executing it
     */
    void invalidate() &&;

    /** Check if the computation is valid
     *
     * @return true if the computation has not been invalidated
     */
    [[nodiscard]] auto valid() const noexcept -> bool;

private:
    auto hasUnconsumedTask() const noexcept -> bool;
    void warnUnconsumedTask() const noexcept;
};
} // namespace openPMD::auxiliary
