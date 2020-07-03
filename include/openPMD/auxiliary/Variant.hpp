/* Copyright 2017-2020 Fabian Koller
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

#if __cplusplus >= 201703L
#   include <variant> // IWYU pragma: export
namespace variantSrc = std;
#else
#   include <mpark/variant.hpp> // IWYU pragma: export
namespace variantSrc = mpark;
#endif

#include <type_traits>


namespace openPMD
{
namespace auxiliary
{
/** Generic object to store a set of datatypes in without losing type safety.
 *
 * @tparam T_DTYPES Enumeration of datatypes to be stored and identified.
 * @tparam T        Varaidic template argument list of datatypes to be stored.
 */
template< class T_DTYPES, typename ... T >
class Variant
{
    static_assert(std::is_enum< T_DTYPES >::value, "Datatypes to Variant must be supplied as enum.");

public:
    using resource = variantSrc::variant< T ... >;
    /** Construct a lightweight wrapper around a generic object that indicates
     * the concrete datatype of the specific object stored.
     *
     * @note    Gerneric objects can only generated implicitly if their datatype
     *          is contained in T_DTYPES.
     * @param   r   Generic object to be stored.
     */
    Variant(resource r)
            : dtype{static_cast<T_DTYPES>(r.index())},
              m_data{r}
    { }

    /** Retrieve a stored specific object of known datatype with ensured type-safety.
     *
     * @throw   std::bad_variant_access if stored object is not of type U.
     * @tparam  U   Type of the object to be retrieved.
     * @return  Copy of the retrieved object of type U.
     */
    template< typename U >
    U get() const
    {
        return variantSrc::get< U >(m_data);
    }

    /** Retrieve the stored generic object.
     *
     * @return  Copy of the stored generic object.
     */
    resource getResource() const
    {
        return m_data;
    }
    
    /** Retrieve the index of the alternative that is currently been held
     *
     * @return  zero-based index
     */
    constexpr size_t index() const noexcept
    {
        return m_data.index();
    }

    T_DTYPES dtype;

protected:
    resource m_data;
};

namespace detail
{
    struct Empty
    {
    };
} // namespace detail

template< typename T >
class Option
{
    using data_t = variantSrc::variant< T, detail::Empty >;
    data_t m_data;

public:
    explicit Option() : m_data( detail::Empty() )
    {
    }
    Option( T data ) : m_data( std::move( data ) )
    {
    }

    Option( Option const & other ) = default;

    Option &
    operator=( Option && other )
    {
        if( other.has_value() )
        {
            m_data.template emplace< 0 >( std::move( other.get() ) );
        }
        else
        {
            m_data.template emplace< 1 >( detail::Empty() );
        }
        return *this;
    }

    Option &
    operator=( Option const & other )
    {
        if( other.has_value() )
        {
            m_data.template emplace< 0 >( other.get() );
        }
        else
        {
            m_data.template emplace< 1 >( detail::Empty() );
        }
        return *this;
    }

    bool
    operator==( Option const & other ) const
    {
        if( has_value() )
        {
            return !other.has_value();
        }
        else
        {
            if( !other.has_value() )
            {
                return false;
            }
            else
            {
                return get() == other.get();
            }
        }
    }

    bool
    operator!=( Option const & other ) const
    {
        return !( *this == other );
    }

    bool
    has_value() const
    {
        return m_data.index() == 0;
    }

    T const &
    get() const
    {
        return variantSrc::template get< T >( m_data );
    }

    T &
    get()
    {
        return variantSrc::template get< T >( m_data );
    }
};
} // namespace auxiliary
} // namespace openPMD
