/* Copyright 2018-2021 Axel Huebl
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
#pragma once

#include <array>
#include <memory>
#include <type_traits>
#include <vector>

namespace openPMD
{
//! @{
/** Share ownership with a raw pointer
 *
 * Helper function to share load/store data ownership
 * unprotected and without reference counting with a
 * raw pointer or stdlib container (that implements a
 * contiguous data storage).
 *
 * @warning this is a helper function to bypass the shared-pointer
 *          API for storing data behind raw pointers. Using it puts
 *          the responsibility of buffer-consistency between stores
 *          and flushes to the users side without an indication via
 *          reference counting.
 */
template <typename T>
std::shared_ptr<T> shareRaw(T *x)
{
    return std::shared_ptr<T>(x, [](T *) {});
}

template <typename T>
std::shared_ptr<T const> shareRaw(T const *x)
{
    return std::shared_ptr<T const>(x, [](T const *) {});
}

template <typename T>
auto shareRaw(T &c)
    -> std::shared_ptr<typename std::remove_pointer<decltype(c.data())>::type>
{
    using value_type = typename std::remove_pointer<decltype(c.data())>::type;
    return std::shared_ptr<value_type>(c.data(), [](value_type *) {});
}

template <typename T>
auto shareRaw(T const &c)
    -> std::shared_ptr<typename std::remove_pointer<decltype(c.data())>::type>
{
    using value_type = typename std::remove_pointer<decltype(c.data())>::type;
    return std::shared_ptr<value_type>(c.data(), [](value_type *) {});
}
//! @}
} // namespace openPMD
