/* Copyright 2017 Fabian Koller
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
// boost::variant uses boost::mpl::list to store template types
// the default length of the list is limited to 20 items,
// there are more dtypes handled in openPMD-api
// BOOST_MPL_LIMIT_LIST_SIZE has to be a multiple of 10
#define BOOST_MPL_CFG_NO_PREPROCESSED_HEADERS
#define BOOST_MPL_LIMIT_LIST_SIZE 30


#include <boost/variant.hpp>


/** Generic object to store a set of datatypes in without losing type safety.
 *
 * @tparam T_DTYPES Enumeration of datatypes to be stored and identified.
 * @tparam T        Varaidic template argument list of datatypes to be stored.
 */
template< class T_DTYPES, typename ... T >
class Variadic
{
    static_assert(std::is_enum< T_DTYPES >::value, "Datatypes to Variadic must be supplied as enum.");

public:
    using resource = boost::variant< T ... >;
    /** Construct a lightweight wrapper around a generic object that indicates
     * the concrete datatype of the specific object stored.
     *
     * @note    Gerneric objects can only generated implicitly if their datatype
     *          is contained in T_DTYPES.
     * @param   r   Generic object to be stored.
     */
    Variadic(resource r)
            : dtype{static_cast<T_DTYPES>(r.which())},
              m_data{r}
    { }

    /** Retrieve a stored specific object of known datatype with ensured type-safety.
     *
     * @throw   boost::bad_get if stored object is not of type U.
     * @tparam  U   Type of the object to be retrieved.
     * @return  Copy of the retrieved object of type U.
     */
    template< typename U >
    U get() const
    {
        return boost::get< U >(m_data);
    }

    /** Retrieve the stored generic object.
     *
     * @return  Copy of the stored generic object.
     */
    resource getResource() const
    {
        return m_data;
    }

    T_DTYPES dtype;

private:
    resource m_data;
};
