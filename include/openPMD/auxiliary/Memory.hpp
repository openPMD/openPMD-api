/* Copyright 2017-2021 Fabian Koller
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

#include "openPMD/Dataset.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

#include <complex>
#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace openPMD
{
namespace auxiliary
{
    std::unique_ptr<void, std::function<void(void *)>>
    allocatePtr(Datatype dtype, uint64_t numPoints);

    std::unique_ptr<void, std::function<void(void *)>>
    allocatePtr(Datatype dtype, Extent const &e);

    /*
     * A buffer for the WRITE_DATASET task that can either be a std::shared_ptr
     * or a std::unique_ptr.
     */
    struct WriteBuffer
    {
        using EligibleTypes = std::
            variant<std::shared_ptr<void const>, UniquePtrWithLambda<void>>;
        EligibleTypes m_buffer;

        WriteBuffer();

        template <typename... Args>
        explicit WriteBuffer(Args &&...args)
            : m_buffer(std::forward<Args>(args)...)
        {}

        WriteBuffer(WriteBuffer &&) noexcept(
            noexcept(EligibleTypes(std::declval<EligibleTypes &&>())));
        WriteBuffer(WriteBuffer const &) = delete;
        WriteBuffer &operator=(WriteBuffer &&) noexcept(noexcept(
            std::declval<EligibleTypes &>() =
                std::declval<EligibleTypes &&>()));
        WriteBuffer &operator=(WriteBuffer const &) = delete;

        WriteBuffer const &operator=(std::shared_ptr<void const> ptr);

        WriteBuffer const &operator=(UniquePtrWithLambda<void const> ptr);

        void const *get() const;
    };
} // namespace auxiliary
} // namespace openPMD
