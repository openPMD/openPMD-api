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

#include <any>
#include <cstddef>
#include <type_traits>

namespace openPMD
{
namespace auxiliary
{
    /** Generic object to store a set of datatypes in without losing type
     * safety.
     *
     * @tparam T_DTYPES Enumeration of datatypes to be stored and identified.
     * @tparam T        Varaidic template argument list of datatypes to be
     * stored.
     */
    template <class T_DTYPES, typename... variant_types>
    class Variant
    {
        static_assert(
            std::is_enum<T_DTYPES>::value,
            "Datatypes to Variant must be supplied as enum.");

    public:
        struct from_basic_type_tag
        {};
        static constexpr from_basic_type_tag from_basic_type =
            from_basic_type_tag{};
        struct from_any_tag
        {};
        static constexpr from_any_tag from_any = from_any_tag{};
        template <typename U>
        Variant(from_basic_type_tag, U);

        Variant(from_any_tag, std::any);

        /** Construct a lightweight wrapper around a generic object that
         * indicates the concrete datatype of the specific object stored.
         *
         * @note    Gerneric objects can only generated implicitly if their
         * datatype is contained in T_DTYPES.
         * @param   u   Generic object to be stored.
         */
        template <typename U>
        Variant(U u);

        /** Retrieve a stored specific object of known datatype with ensured
         * type-safety.
         *
         * @throw   std::bad_variant_access if stored object is not of type
         * U.
         * @tparam  U   Type of the object to be retrieved.
         * @return  Copy of the retrieved object of type U.
         */
        template <typename U>
        [[nodiscard]] U const &get() const;

        /** Retrieve the stored generic object.
         *
         * @return  Copy of the stored generic object.
         */
        template <typename variant_t>
        [[nodiscard]] variant_t const &getVariant() const
        {
            return *std::any_cast<variant_t>(&m_data);
        }

        [[nodiscard]] std::any const &getAny() const
        {
            return m_data;
        }

        /** Retrieve the index of the alternative that is currently been held
         *
         * @return  zero-based index
         */
        [[nodiscard]] size_t index() const;

        T_DTYPES dtype;

    private:
        std::any m_data;
    };

    /*
     * Helper type for std::visit,
     * see https://en.cppreference.com/w/cpp/utility/variant/visit
     */
    template <class... Ts>
    struct overloaded : Ts...
    {
        using Ts::operator()...;
    };
    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;
} // namespace auxiliary
} // namespace openPMD
