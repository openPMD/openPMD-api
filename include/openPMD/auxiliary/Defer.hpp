#pragma once

#include <type_traits>
#include <utility>

namespace openPMD::auxiliary
{
template <typename F>
struct defer_type
{
    F functor;
    ~defer_type()
    {
        std::move(functor)();
    }
};

template <typename F>
auto defer(F &&functor) -> defer_type<std::remove_reference_t<F>>
{
    return defer_type<std::remove_reference_t<F>>{std::forward<F>(functor)};
}
} // namespace openPMD::auxiliary
