/* Copyright 2017-2025 Fabian Koller, Axel Huebl, Franz Poeschel
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
#include "openPMD/Error.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"

#include <any>
#include <functional>
#include <memory>
#include <utility>

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
        /*
         * Sic. Have to put the unique_ptr behind a shared_ptr because
         * std::variant does not want non-copyable types.
         * Use a separate class to avoid mistakes in double dereference.
         */
        struct CopyableUniquePtr
            : private std::shared_ptr<UniquePtrWithLambda<void>>
        {
        private:
            using parent_t = std::shared_ptr<UniquePtrWithLambda<void>>;

        public:
            CopyableUniquePtr();
            CopyableUniquePtr(UniquePtrWithLambda<void> ptr_in);
            auto get() -> void *;
            [[nodiscard]] auto get() const -> void const *;
            [[nodiscard]] auto release() -> UniquePtrWithLambda<void>;
        };
        using SharedPtr = std::shared_ptr<void const>;
        /*
         * Use std::any publically since some compilers have trouble with
         * certain uses of std::variant, so hide it from them.
         * Look into Memory_internal.hpp for the variant type.
         * https://github.com/openPMD/openPMD-api/issues/1720
         */
        std::any m_buffer;

        WriteBuffer();
        WriteBuffer(std::shared_ptr<void const> ptr);
        WriteBuffer(UniquePtrWithLambda<void> ptr);

        WriteBuffer(WriteBuffer &&) noexcept;
        WriteBuffer(WriteBuffer const &) = delete;
        WriteBuffer &operator=(WriteBuffer &&) noexcept;
        WriteBuffer &operator=(WriteBuffer const &) = delete;

        WriteBuffer const &operator=(std::shared_ptr<void const> ptr);
        WriteBuffer const &operator=(UniquePtrWithLambda<void> ptr);

        void const *get() const;

        template <typename variant_t>
        auto as_variant() -> variant_t &
        {
            return *std::any_cast<variant_t>(&m_buffer);
        }

        template <typename variant_t>
        auto as_variant() const -> variant_t const &
        {
            return *std::any_cast<variant_t>(&m_buffer);
        }
    };
} // namespace auxiliary
} // namespace openPMD
