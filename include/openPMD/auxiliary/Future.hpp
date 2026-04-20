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
    task_type m_task;
    bool m_task_valid = true;

    auto operator()() -> T;
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

    DeferredComputation(DeferredComputation &&) noexcept;
    DeferredComputation(DeferredComputation const &) = delete;

    auto operator=(DeferredComputation &&) noexcept -> DeferredComputation &;
    auto operator=(DeferredComputation const &)
        -> DeferredComputation & = delete;

    ~DeferredComputation();

    /** Get the result of the computation
     *
     * @return The result of the computation
     */
    auto get() -> T;
    /** Invoke the computation
     *
     * Alias for get()
     * @return The result of the computation
     */
    auto operator()() -> T;

    /** Discard the computation without executing it
     */
    void forget() &&;

    /** Check if the computation is valid
     *
     * @return true if the computation has not been forgotten
     */
    [[nodiscard]] auto valid() const noexcept -> bool;
};
} // namespace openPMD::auxiliary
