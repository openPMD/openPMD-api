#pragma once

#include <optional>
#include <type_traits>

namespace openPMD::auxiliary
{
template <typename Optional, typename F>
auto optional_and_then(Optional &&arg, F &&then)
    -> std::invoke_result_t<F, decltype(*std::declval<Optional &&>())>
{
    if (!arg.has_value())
    {
        return std::nullopt;
    }
    return std::forward<F>(then)(*std::forward<Optional>(arg));
}
} // namespace openPMD::auxiliary
