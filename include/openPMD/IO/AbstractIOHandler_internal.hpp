#pragma once

#include "openPMD/IO/AbstractIOHandler.hpp"

#include <variant>

namespace openPMD::internal
{
struct AbstractIOHandlerInitFrom
    : std::variant<GlobalParameters, AbstractIOHandler *>
{
    using Left = GlobalParameters;
    using Right = AbstractIOHandler *;
    using parent_t = std::variant<Left, Right>;
    using parent_t::parent_t;
    inline auto as_parent() -> parent_t &
    {
        return *this;
    }
    [[nodiscard]] inline auto as_parent() const -> parent_t const &
    {
        return *this;
    }
    auto asGlobalParameters() -> GlobalParameters &;
    [[nodiscard]] auto asGlobalParameters() const -> GlobalParameters const &;
};
} // namespace openPMD::internal
