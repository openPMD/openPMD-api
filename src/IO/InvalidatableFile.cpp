/* Copyright 2017-2025 Franz Poeschel., Axel Huebl, Luca Fedeli
 *
 * This file is part of openPMD-api.
 *
 * openPMD-api is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * openPMD-api is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with openPMD-api.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "openPMD/IO/InvalidatableFile.hpp"

namespace openPMD::internal
{
FileState::FileState(std::string name_in) : name(std::move(name_in))
{}
auto SharedFileState::has_value() const -> bool
{
    return ptr_type::operator bool() && ptr_type::operator*().has_value();
}
SharedFileState::operator bool() const
{
    return has_value();
}
auto SharedFileState::operator*() -> FileState &
{
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *ptr_type::operator*();
}
auto SharedFileState::operator->() -> FileState *
{
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return &*ptr_type::operator*();
}
auto SharedFileState::operator*() const -> FileState const &
{
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return *ptr_type::operator*();
}
auto SharedFileState::operator->() const -> FileState const *
{
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return &*ptr_type::operator*();
}
void SharedFileState::reset_optional()
{
    ptr_type::operator*().reset();
}
} // namespace openPMD::internal

auto std::less<openPMD::internal::SharedFileState>::operator()(
    first_argument_type const &first, second_argument_type const &second) const
    -> result_type
{
    if (!first || !second)
    {
        return std::less<>()(first.get(), second.get());
    }
    // If possible, compare by name
    return less<>()((*first).name, (*second).name);
}
