#pragma once

#include "openPMD/IO/Access.hpp"
#include <type_traits>

namespace openPMD::internal
{
template <typename Child> // CRT
class ScientificDefaults
{
protected:
    void finalize(Access);
};

template <typename Child>
constexpr bool HasScientificDefaults_v =
    std::is_base_of_v<ScientificDefaults<Child>, Child>;
} // namespace openPMD::internal
