/* Copyright 2020-2021 Franz Poeschel
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

#include "VariantSrc.hpp"

#include <type_traits>
#include <utility> // std::forward, std::move

namespace openPMD
{
namespace auxiliary
{
    namespace detail
    {
        struct Empty
        {};
    } // namespace detail

    /**
     * @brief Simple Option type based on variantSrc::variant.
     *
     * @tparam T Type that can be optionally stored in an Optional object.
     */
    template <typename T>
    class Option
    {
        using data_t = variantSrc::variant<T, detail::Empty>;
        data_t m_data;

    public:
        /** Create an empty Option.
         */
        explicit Option() : m_data(detail::Empty())
        {}

        /** Create a full Option.
         *
         * @param data The object to emplace in the Option.
         */
        Option(T data) : m_data(std::move(data))
        {}

        Option(Option const &other) = default;

        Option &operator=(Option &&other)
        {
            if (other.has_value())
            {
                m_data.template emplace<0>(std::move(other.get()));
            }
            else
            {
                m_data.template emplace<1>(detail::Empty());
            }
            return *this;
        }

        Option &operator=(Option const &other)
        {
            if (other.has_value())
            {
                m_data.template emplace<0>(other.get());
            }
            else
            {
                m_data.template emplace<1>(detail::Empty());
            }
            return *this;
        }

        bool operator==(Option const &other) const
        {
            if (has_value())
            {
                return !other.has_value();
            }
            else
            {
                if (!other.has_value())
                {
                    return false;
                }
                else
                {
                    return get() == other.get();
                }
            }
        }

        bool operator!=(Option const &other) const
        {
            return !(*this == other);
        }

        /**
         * @return Is an object constantly stored in this?
         */
        bool has_value() const
        {
            return m_data.index() == 0;
        }

        /**
         * @return Is an object constantly stored in this?
         */
        operator bool() const
        {
            return has_value();
        }

        /**
         * @brief Access the emplaced object if one is present.
         *
         * @throw std::bad_variant_access if no object is present.
         * @return The emplaced object.
         */
        T const &get() const
        {
            return variantSrc::template get<T>(m_data);
        }

        /**
         * @brief Access the emplaced object if one is present.
         *
         * @throw std::bad_variant_access if no object is present.
         * @return The emplaced object.
         */
        T &get()
        {
            return variantSrc::template get<T>(m_data);
        }
    };

    template <typename T>
    Option<typename std::decay<T>::type> makeOption(T &&val)
    {
        return Option<typename std::decay<T>::type>(std::forward<T>(val));
    }
} // namespace auxiliary
} // namespace openPMD
