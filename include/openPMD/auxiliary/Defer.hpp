#pragma once

#include <functional>
#include <type_traits>
#include <utility>

namespace openPMD::auxiliary
{
/** Defer wrapper
 *
 * Executes a functor when destroyed unless explicitly cancelled.
 * Similar to Go's defer or C++'s experimental::scope_exit.
 *
 * Similar also to DeferredComputation under Future.hpp, but has another
 * application scope (this: internal resource cleanup, that: public Future-like
 * API) and is hence kept separate.
 *
 * @tparam F The functor type
 */
template <typename F>
struct defer_type
{
    F functor;
    bool do_run_this = true;
    ~defer_type()
    {
        if (!do_run_this)
        {
            return;
        }
        do_run_this = false;
        std::move(functor)();
    }

    explicit defer_type() : do_run_this(false)
    {}

    struct forwarding_tag
    {};

    template <typename F_>
    defer_type(forwarding_tag, F_ &&functor_in)
        : functor{std::forward<F_>(functor_in)}
    {}

    template <typename F_>
    defer_type(defer_type<F_> &&other)
        : functor{std::move(other.functor)}, do_run_this(other.do_run_this)
    {
        other.do_run_this = false;
    }

    template <typename F_>
    auto operator=(defer_type<F_> &&other)
    {
        functor = std::move(other.functor);
        do_run_this = other.do_run_this;
        other.do_run_this = false;
    }

    defer_type(defer_type const &) = delete;
    auto operator=(defer_type const &) -> defer_type & = delete;
};

/** Type-erased defer wrapper for void functors */
using opaque_defer_type = defer_type<std::function<void()>>;

/** Create a defer wrapper
 *
 * Creates a defer wrapper that will execute the given functor when
 * destroyed.
 *
 * @param functor The functor to execute on destruction
 * @return A defer wrapper
 */
template <typename F>
auto defer(F &&functor) -> defer_type<std::remove_reference_t<F>>
{
    using res_t = defer_type<std::remove_reference_t<F>>;
    using tag_t = typename res_t::forwarding_tag;
    return res_t{tag_t{}, std::forward<F>(functor)};
}
} // namespace openPMD::auxiliary
