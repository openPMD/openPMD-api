/* Copyright 2020-2021 Axel Huebl
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

#include <exception>

namespace openPMD
{
namespace auxiliary
{
    /** Returns a value reference stored in a dynamically casted pointer
     *
     * Safe version of *dynamic_cast< New_Type* >( some_ptr ); This function
     * will throw as dynamic_cast and will furthermore throw if the result
     * of the dynamic_cast is a nullptr.
     *
     * @tparam New_Type new type to cast to
     * @tparam Old_Type old type to cast from
     * @param[in] ptr and input pointer type
     * @return value reference of a dereferenced, dynamically casted ptr to
     * New_Type*
     */
    template <typename New_Type, typename Old_Type>
    inline New_Type &deref_dynamic_cast(Old_Type *ptr)
    {
        auto const tmp_ptr = dynamic_cast<New_Type *>(ptr);
        if (tmp_ptr == nullptr)
            throw std::runtime_error("Dynamic cast returned a nullptr!");
        return *tmp_ptr;
    }

} // namespace auxiliary
} // namespace openPMD
