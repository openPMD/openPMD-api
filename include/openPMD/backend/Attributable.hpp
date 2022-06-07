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

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/auxiliary/OutOfRangeMsg.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/Writable.hpp"

#include <cstddef>
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#define OPENPMD_protected protected
#endif

namespace openPMD
{
namespace traits
{
    template <typename T>
    struct GenerationPolicy;
} // namespace traits
class AbstractFilePosition;
class AttributableInterface;
class Iteration;
namespace internal
{
    class SeriesInternal;
}

class no_such_attribute_error : public std::runtime_error
{
public:
    no_such_attribute_error(std::string const &what_arg)
        : std::runtime_error(what_arg)
    {}
    virtual ~no_such_attribute_error()
    {}
};

namespace internal
{
    class AttributableData
    {
        friend class openPMD::AttributableInterface;

    public:
        AttributableData();
        AttributableData(AttributableData const &) = delete;
        AttributableData(AttributableData &&) = delete;
        virtual ~AttributableData() = default;

        AttributableData &operator=(AttributableData const &) = delete;
        AttributableData &operator=(AttributableData &&) = delete;

        using A_MAP = std::map<std::string, Attribute>;
        Writable m_writable;

    private:
        A_MAP m_attributes;
    };

    enum class SetAttributeMode : char
    {
        WhileReadingAttributes,
        FromPublicAPICall
    };

    /** Verify values of attributes in the frontend
     *
     * verify string attributes are not empty (backend restriction, e.g., HDF5)
     */
    template <typename T>
    inline void attr_value_check(
        std::string const /* key */, T /* value */, SetAttributeMode)
    {}

    template <>
    inline void attr_value_check(
        std::string const key, std::string const value, SetAttributeMode mode)
    {
        switch (mode)
        {
        case SetAttributeMode::FromPublicAPICall:
            if (value.empty())
                throw std::runtime_error(
                    "[setAttribute] Value for string attribute '" + key +
                    "' must not be empty!");
            break;
        case SetAttributeMode::WhileReadingAttributes:
            // no checks while reading
            break;
        }
    }

} // namespace internal

/** @brief Layer to manage storage of attributes associated with file objects.
 *
 * Mandatory and user-defined Attributes and their data for every object in the
 * openPMD hierarchy are stored and managed through this class.
 */
class AttributableInterface
{
    // @todo remove unnecessary friend (wew that sounds bitter)
    using A_MAP = std::map<std::string, Attribute>;
    friend Writable *getWritable(AttributableInterface *);
    template <typename T_elem>
    friend class BaseRecord;
    template <typename T, typename T_key, typename T_container>
    friend class Container;
    template <typename T>
    friend struct traits::GenerationPolicy;
    friend class Iteration;
    friend class Series;
    friend class SeriesInterface;
    friend class Writable;
    friend class WriteIterations;

protected:
    internal::AttributableData *m_attri = nullptr;

    // Should not be called publicly, only by implementing classes
    AttributableInterface(internal::AttributableData *);
    template <typename T>
    AttributableInterface(T *attri)
        : AttributableInterface{
              static_cast<internal::AttributableData *>(attri)}
    {}

public:
    virtual ~AttributableInterface() = default;

    /** Populate Attribute of provided name with provided value.
     *
     * @note If the provided Attribute already exists, the value is replaced.
     *       If it does not exist, a new Attribute is created.
     *
     * @tparam  T       Type of the object to be stored. Only types contained in
     * Datatype can be handled.
     * @param   key     Key (i.e. name) to identify and store an Attributes
     * value by.
     * @param   value   Value of Attribute stored with the provided key.
     * @return  true if key was already present, false otherwise
     *
     * @{
     */
    template <typename T>
    bool setAttribute(std::string const &key, T value);
    bool setAttribute(std::string const &key, char const value[]);
    /** @}
     */

    /** Retrieve value of Attribute stored with provided key.
     *
     * @throw   no_such_attribute_error If no Attribute is currently stored with
     * the provided key.
     * @param   key Key (i.e. name) of the Attribute to retrieve value for.
     * @return  Stored Attribute in Variant form.
     */
    Attribute getAttribute(std::string const &key) const;

    /** Remove Attribute of provided value both logically and physically.
     *
     * @param   key Key (i.e. name) of the Attribute to remove.
     * @return  true if provided key was present and removal succeeded, false
     * otherwise.
     */
    bool deleteAttribute(std::string const &key);

    /** List all currently stored Attributes' keys.
     *
     * @return  Vector of keys (i.e. names) of all currently stored Attributes.
     */
    std::vector<std::string> attributes() const;
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
    bool containsAttribute(std::string const &key) const;

    /** Retrieve a user-supplied comment associated with the object.
     *
     * @throw   no_such_attribute_error If no comment is currently stored.
     * @return  String containing the user-supplied comment.
     */
    std::string comment() const;
    /** Populate Attribute corresponding to a comment with the user-supplied
     * comment.
     *
     * @param   comment String value to be stored as a comment.
     * @return  Reference to modified Attributable.
     */
    AttributableInterface &setComment(std::string const &comment);

    /** Flush the corresponding Series object
     *
     * Writable connects all objects of an openPMD series through a linked list
     * of parents. This method will walk up the parent list until it reaches
     * an object that has no parent, which is the Series object, and flush()-es
     * it.
     */
    void seriesFlush();

    /** String serialization to describe an Attributable
     *
     * This object contains the Series data path as well as the openPMD object
     * names to find an Attributable. This can be used to re-open a Series
     * and re-constructing an Attributable, e.g. on a remote context/node.
     */
    struct MyPath
    {
        std::string directory; //! e.g., samples/git-samples/
        std::string seriesName; //! e.g., data%T
        std::string seriesExtension; //! e.g., .bp, .h5, .json, ...
        /** A vector of openPMD object names
         *
         * Indicates where this Attributable may be found within its Series.
         * Prefixed by the accessed object, e.g.,
         *   "iterations", "100", "meshes", "E", "x"
         * Notice that RecordComponent::SCALAR is included in this list, too.
         */
        std::vector<std::string> group;

        /** Reconstructs a path that can be passed to a Series constructor */
        std::string filePath() const;
    };

    /**
     * @brief The path to this object within its containing Series.
     *
     * @return A struct informing about the context of this Attributable.
     */
    MyPath myPath() const;

    OPENPMD_protected:

    internal::SeriesInternal const & retrieveSeries() const;
    internal::SeriesInternal &retrieveSeries();

    /** Returns the corresponding Iteration
     *
     * Return the openPMD::iteration that this Attributable is contained in.
     * This walks up the linked parents until it finds the Iteration object.
     * Throws an error otherwise, e.g., for Series objects.
     * @{
     */
    Iteration const &containingIteration() const;
    Iteration &containingIteration();
    /** @} */

    void seriesFlush(internal::FlushParams);

    void flushAttributes(internal::FlushParams const &);

    template <typename T>
    bool setAttributeImpl(
        std::string const &key, T value, internal::SetAttributeMode);
    bool setAttributeImpl(
        std::string const &key, char const value[], internal::SetAttributeMode);

    enum ReadMode
    {
        /**
         * Don't read an attribute from the backend if it has been previously
         * read.
         */
        IgnoreExisting,
        /**
         * Read all the attributes that the backend has to offer and override
         * if it has been read previously.
         */
        OverrideExisting,
        /**
         * Remove all attributes that have been read previously and read
         * everything that the backend currently has to offer.
         */
        FullyReread
    };
    void readAttributes(ReadMode);

    /** Retrieve the value of a floating point Attribute of user-defined
     * precision with ensured type-safety.
     *
     * @note    Since the precision of certain Attributes is intentionally left
     *          unspecified in the openPMD standard, this provides a mechanism
     * to retrieve those values without giving up type-safety.
     * @see
     * https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#conventions-throughout-these-documents
     * @note    If the supplied and stored floating point precision are not the
     *          same, the value is cast to the desired precision
     * unconditionally.
     *
     * @throw   no_such_attribute_error If no Attribute is currently stored with
     * the provided key.
     * @tparam  T   Floating point type of user-defined precision to retrieve
     * the value as.
     * @param   key Key (i.e. name) of the floating-point Attribute to retrieve
     * value for.
     * @return  Value of stored Attribute as supplied floating point type.
     */
    template <typename T>
    T readFloatingpoint(std::string const &key) const;
    /** Retrieve a vector of values of a floating point Attributes of
     * user-defined precision with ensured type-safety.
     *
     * @note    Since the precision of certain Attributes is intentionally left
     *          unspecified in the openPMD standard, this provides a mechanism
     * to retrieve those values without giving up type-safety.
     * @see
     * https://github.com/openPMD/openPMD-standard/blob/latest/STANDARD.md#conventions-throughout-these-documents
     * @note    If the supplied and stored floating point precision are not the
     *          same, the values are cast to the desired precision
     * unconditionally.
     *
     * @throw   no_such_attribute_error If no Attribute is currently stored with
     * the provided key.
     * @tparam  T   Floating point type of user-defined precision to retrieve
     * the values as.
     * @param   key Key (i.e. name) of the floating-point Attribute to retrieve
     * values for.
     * @return  Vector of values of stored Attribute as supplied floating point
     * type.
     */
    template <typename T>
    std::vector<T> readVectorFloatingpoint(std::string const &key) const;

    /* views into the resources held by m_writable
     * purely for convenience so code that uses these does not have to go
     * through m_writable-> */
    AbstractIOHandler *IOHandler()
    {
        return m_attri->m_writable.IOHandler.get();
    }
    AbstractIOHandler const *IOHandler() const
    {
        return m_attri->m_writable.IOHandler.get();
    }
    Writable *&parent()
    {
        return m_attri->m_writable.parent;
    }
    Writable const *parent() const
    {
        return m_attri->m_writable.parent;
    }
    Writable &writable()
    {
        return m_attri->m_writable;
    }
    Writable const &writable() const
    {
        return m_attri->m_writable;
    }

    inline internal::AttributableData &get()
    {
        if (m_attri)
        {
            return *m_attri;
        }
        else
        {
            throw std::runtime_error(
                "[AttributableInterface] "
                "Cannot use default-constructed Attributable.");
        }
    }
    inline internal::AttributableData const &get() const
    {
        if (m_attri)
        {
            return *m_attri;
        }
        else
        {
            throw std::runtime_error(
                "[AttributableInterface] "
                "Cannot use default-constructed Attributable.");
        }
    }

    bool dirty() const
    {
        return writable().dirty;
    }
    bool &dirty()
    {
        return writable().dirty;
    }
    bool written() const
    {
        return writable().written;
    }
    bool &written()
    {
        return writable().written;
    }

private:
    /**
     * @brief Link with parent.
     *
     * @param w The Writable representing the parent.
     */
    virtual void linkHierarchy(Writable &w);
}; // AttributableInterface

// Alias this as Attributable since this is a public abstract parent class
// for most of the classes in our object model of the openPMD hierarchy
using Attributable = AttributableInterface;

class LegacyAttributable : public AttributableInterface
{
protected:
    std::shared_ptr<internal::AttributableData> m_attributableData =
        std::make_shared<internal::AttributableData>();

public:
    LegacyAttributable() : AttributableInterface{nullptr}
    {
        AttributableInterface::m_attri = m_attributableData.get();
    }
};

template <typename T>
inline bool AttributableInterface::setAttribute(std::string const &key, T value)
{
    return setAttributeImpl(
        key, std::move(value), internal::SetAttributeMode::FromPublicAPICall);
}

inline bool
Attributable::setAttribute(std::string const &key, char const value[])
{
    return setAttributeImpl(
        key, value, internal::SetAttributeMode::FromPublicAPICall);
}

// note: we explicitly instantiate Attributable::setAttributeImpl for all T in
// Datatype in Attributable.cpp
template <typename T>
inline bool Attributable::setAttributeImpl(
    std::string const &key,
    T value,
    internal::SetAttributeMode setAttributeMode)
{
    internal::attr_value_check(key, value, setAttributeMode);

    auto &attri = get();
    if (IOHandler() && Access::READ_ONLY == IOHandler()->m_frontendAccess)
    {
        auxiliary::OutOfRangeMsg const out_of_range_msg(
            "Attribute", "can not be set (read-only).");
        throw no_such_attribute_error(out_of_range_msg(key));
    }

    dirty() = true;
    auto it = attri.m_attributes.lower_bound(key);
    if (it != attri.m_attributes.end() &&
        !attri.m_attributes.key_comp()(key, it->first))
    {
        // key already exists in map, just replace the value

        // note: due to a C++17 issue with NVCC 11.0.2 we write the
        //       T value to variant conversion explicitly
        //       https://github.com/openPMD/openPMD-api/pull/1103
        // it->second = Attribute(std::move(value));
        it->second = Attribute(Attribute::resource(std::move(value)));
        return true;
    }
    else
    {
        // emplace a new map element for an unknown key
        attri.m_attributes.emplace_hint(
            it, std::make_pair(key, Attribute(std::move(value))));
        return false;
    }
}

inline bool Attributable::setAttributeImpl(
    std::string const &key,
    char const value[],
    internal::SetAttributeMode setAttributeMode)
{
    return this->setAttributeImpl(key, std::string(value), setAttributeMode);
}

template <typename T>
inline T AttributableInterface::readFloatingpoint(std::string const &key) const
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    return getAttribute(key).get<T>();
}

template <typename T>
inline std::vector<T>
AttributableInterface::readVectorFloatingpoint(std::string const &key) const
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    return getAttribute(key).get<std::vector<T> >();
}
} // namespace openPMD
