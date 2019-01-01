/* Copyright 2017-2019 Fabian Koller
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

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/auxiliary/OutOfRangeMsg.hpp"

#include <exception>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <cstddef>

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#   define OPENPMD_protected protected
#endif


namespace openPMD
{
namespace traits
{
    template< typename T >
    struct GenerationPolicy;
} // traits
class AbstractFilePosition;

class no_such_attribute_error : public std::runtime_error
{
public:
    no_such_attribute_error(std::string const& what_arg)
            : std::runtime_error(what_arg)
    { }
    virtual ~no_such_attribute_error() { }
};


/** @brief Layer to manage storage of attributes associated with file objects.
 *
 * Mandatory and user-defined Attributes and their data for every object in the
 * openPMD hierarchy are stored and managed through this class.
 */
class Attributable
{
    using A_MAP = std::map< std::string, Attribute >;
    friend Writable* getWritable(Attributable*);
    template< typename T_elem >
    friend class BaseRecord;
    template<
        typename T,
        typename T_key,
        typename T_container
    >
    friend class Container;
    template< typename T >
    friend struct traits::GenerationPolicy;
    friend class Iteration;
    friend class Series;

public:
    Attributable();
    Attributable(Attributable const&);
    Attributable(Attributable&&) = delete;
    virtual ~Attributable() = default;

    Attributable& operator=(Attributable const&);
    Attributable& operator=(Attributable&&) = delete;

    /** Populate Attribute of provided name with provided value.
     *
     * @note If the provided Attribute already exists, the value is replaced.
     *       If it does not exist, a new Attribute is created.
     *
     * @tparam  T       Type of the object to be stored. Only types contained in Datatype can be handled.
     * @param   key     Key (i.e. name) to identify and store an Attributes value by.
     * @param   value   Value of Attribute stored with the provided key.
     * @return  true if key was already present, false otherwise
     *
     * @{
     */
    template< typename T >
    bool setAttribute(std::string const& key, T const& value);
    bool setAttribute(std::string const& key, char const value[]);
    /** @}
     */

    /** Retrieve value of Attribute stored with provided key.
     *
     * @throw   no_such_attribute_error If no Attribute is currently stored with the provided key.
     * @param   key Key (i.e. name) of the Attribute to retrieve value for.
     * @return  Stored Attribute in Variant form.
     */
    Attribute getAttribute(std::string const& key) const;

    /** Remove Attribute of provided value both logically and physically.
     *
     * @param   key Key (i.e. name) of the Attribute to remove.
     * @return  true if provided key was present and removal succeeded, false otherwise.
     */
    bool deleteAttribute(std::string const& key);

    /** List all currently stored Attributes' keys.
     *
     * @return  Vector of keys (i.e. names) of all currently stored Attributes.
     */
    std::vector< std::string > attributes() const;
    /** Count all currently stored Attributes.
     *
     * @return  Number of currently stored Attributes.
     */
    size_t numAttributes() const;
    /** Check whether am Attribute with a given key exists.
     *
     * @param   key Key (i.e. name) of the Attribute to find.
     * @return  true if provided key was present, false otherwise.
     */
    bool containsAttribute(std::string const& key) const;

    /** Retrieve a user-supplied comment associated with the object.
     *
     * @throw   no_such_attribute_error If no comment is currently stored.
     * @return  String containing the user-supplied comment.
     */
    std::string comment() const;
    /** Populate Attribute corresponding to a comment with the user-supplied comment.
     *
     * @param   comment String value to be stored as a comment.
     * @return  Reference to modified Attributable.
     */
    Attributable& setComment(std::string const& comment);

OPENPMD_protected:
    void flushAttributes();
    void readAttributes();

    /** Retrieve the value of a floating point Attribute of user-defined precision with ensured type-safety.
     *
     * @note    Since the precision of certain Attributes is intentionally left
     *          unspecified in the openPMD standard, this provides a mechnism to
     *          retrieve those values without giving up type-safety.
     * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#conventions-throughout-these-documents
     * @note    If the supplied and stored floating point precision are not the
     *          same, the value is cast to the desired precision unconditionally.
     *
     * @throw   no_such_attribute_error If no Attribute is currently stored with the provided key.
     * @tparam  T   Floating point type of user-defined precision to retrieve the value as.
     * @param   key Key (i.e. name) of the floating-point Attribute to retrieve value for.
     * @return  Value of stored Attribute as supplied floating point type.
     */
    template< typename T >
    T readFloatingpoint(std::string const& key) const;
    /** Retrieve a vector of values of a floating point Attributes of user-defined precision with ensured type-safety.
     *
     * @note    Since the precision of certain Attributes is intentionally left
     *          unspecified in the openPMD standard, this provides a mechnism to
     *          retrieve those values without giving up type-safety.
     * @see https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#conventions-throughout-these-documents
     * @note    If the supplied and stored floating point precision are not the
     *          same, the values are cast to the desired precision unconditionally.
     *
     * @throw   no_such_attribute_error If no Attribute is currently stored with the provided key.
     * @tparam  T   Floating point type of user-defined precision to retrieve the values as.
     * @param   key Key (i.e. name) of the floating-point Attribute to retrieve values for.
     * @return  Vector of values of stored Attribute as supplied floating point type.
     */
    template< typename T >
    std::vector< T > readVectorFloatingpoint(std::string const& key) const;

    std::shared_ptr< Writable > m_writable;
    /* views into the resources held by m_writable
     * purely for convenience so code that uses these does not have to go through m_wriable-> */
    AbstractFilePosition* abstractFilePosition;
    AbstractIOHandler* IOHandler;
    Writable* parent;
    bool& dirty;
    bool& written;

private:
    virtual void linkHierarchy(std::shared_ptr< Writable > const& w);

    std::shared_ptr< A_MAP > m_attributes;
}; // Attributable

//TODO explicitly instanciate Attributable::setAttribute for all T in Datatype
template< typename T >
inline bool
Attributable::setAttribute(std::string const& key, T const& value)
{
    if( IOHandler && AccessType::READ_ONLY == IOHandler->accessType )
    {
        auxiliary::OutOfRangeMsg const out_of_range_msg(
            "Attribute",
            "can not be set (read-only)."
        );
        throw no_such_attribute_error(out_of_range_msg(key));
    }

    dirty = true;
    auto it = m_attributes->lower_bound(key);
    if( it != m_attributes->end() && !m_attributes->key_comp()(key, it->first) )
    {
        // key already exists in map, just replace the value
        it->second = Attribute(value);
        return true;
    } else
    {
        // emplace a new map element for an unknown key
        m_attributes->emplace_hint(it,
                                   std::make_pair(key, Attribute(value)));
        return false;
    }
}
inline bool
Attributable::setAttribute(std::string const& key, char const value[])
{
    return this->setAttribute(key, std::string(value));
}

template< typename T >
inline T
Attributable::readFloatingpoint(std::string const& key) const
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    return getAttribute(key).get< T >();
}

template< typename T >
inline std::vector< T >
Attributable::readVectorFloatingpoint(std::string const& key) const
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    return getAttribute(key).get< std::vector< T > >();
}
} // namespace openPMD
