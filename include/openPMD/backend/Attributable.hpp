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
#include "openPMD/ThrowError.hpp"
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
#define OPENPMD_protected protected:
#endif

namespace openPMD
{
namespace traits
{
    template <typename T>
    struct GenerationPolicy;
} // namespace traits
class AbstractFilePosition;
class Attributable;
class Iteration;
class Series;

namespace internal
{
    class IterationData;
    class SeriesData;

    class AttributableData
    {
        friend class openPMD::Attributable;

    public:
        AttributableData();
        AttributableData(AttributableData const &) = delete;
        AttributableData(AttributableData &&) = delete;
        virtual ~AttributableData() = default;

        AttributableData &operator=(AttributableData const &) = delete;
        AttributableData &operator=(AttributableData &&) = delete;

        using A_MAP = std::map<std::string, Attribute>;
        /**
         * The Writable associated with this Attributable.
         * There is a one-to-one relation between Attributable and Writable
         * objects, Writable captures the part that backends can see.
         */
        Writable m_writable;

        template <typename T>
        T asInternalCopyOf()
        {
            auto *self = dynamic_cast<typename T::Data_t *>(this);
            if (!self)
            {
                if constexpr (std::is_same_v<Series, T>)
                {
                    throw std::runtime_error(
                        "[Attributable::retrieveSeries] Error when trying to "
                        "retrieve the Series object. Note: An instance of the "
                        "Series object must still exist when flushing. A "
                        "common cause for this error is using a flush call on "
                        "a handle (e.g. `Iteration::seriesFlush()`) when the "
                        "original Series object has already gone out of "
                        "scope.");
                }
                else
                {
                    throw std::runtime_error(

                        "[AttributableData::asInternalCopyOf<T>] Error when "
                        "trying to retrieve a containing object. Note: An "
                        "instance of the Series object must still exist when "
                        "flushing. A common cause for this error is using a "
                        "flush call on a handle (e.g. "
                        "`Iteration::seriesFlush()`) when the original Series "
                        "object has already gone out of scope.");
                }
            }
            T res;
            res.setData(
                std::shared_ptr<typename T::Data_t>(self, [](auto const *) {}));
            return res;
        }

    private:
        /**
         * The attributes defined by this Attributable.
         */
        A_MAP m_attributes;
    };

    template <typename, typename>
    class BaseRecordData;

    class RecordComponentData;

    /*
     * Internal function to turn a handle into an owning handle that will keep
     * not only itself, but the entire Series alive. Works by hiding a copy of
     * the Series into the destructor lambda of the internal shared pointer. The
     * returned handle is entirely safe to use in just the same ways as a normal
     * handle, just the surrounding Series needs not be kept alive any more
     * since it is stored within the handle. By storing the Series in the
     * handle, not in the actual data, reference cycles are avoided.
     *
     * Instantiations for T exist for types RecordComponent,
     * MeshRecordComponent, Mesh, Record, ParticleSpecies, Iteration.
     */
    template <typename T>
    T &makeOwning(T &self, Series);
} // namespace internal

namespace debug
{
    void printDirty(Series const &);
}

/** @brief Layer to manage storage of attributes associated with file objects.
 *
 * Mandatory and user-defined Attributes and their data for every object in the
 * openPMD hierarchy are stored and managed through this class.
 */
class Attributable
{
    // @todo remove unnecessary friend (wew that sounds bitter)
    using A_MAP = std::map<std::string, Attribute>;
    friend Writable *getWritable(Attributable *);
    template <typename T_elem>
    friend class BaseRecord;
    template <typename T_elem>
    friend class BaseRecordInterface;
    template <typename, typename>
    friend class internal::BaseRecordData;
    template <typename T, typename T_key, typename T_container>
    friend class Container;
    template <typename T>
    friend struct traits::GenerationPolicy;
    friend class Iteration;
    friend class Series;
    friend class Writable;
    friend class WriteIterations;
    friend class internal::RecordComponentData;
    friend void debug::printDirty(Series const &);
    template <typename T>
    friend T &internal::makeOwning(T &self, Series);

protected:
    // tag for internal constructor
    struct NoInit
    {};

    using Data_t = internal::AttributableData;
    std::shared_ptr<Data_t> m_attri;

public:
    Attributable();
    Attributable(NoInit);

    virtual ~Attributable() = default;

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
    Attributable &setComment(std::string const &comment);

    /** Flush the corresponding Series object
     *
     * Writable connects all objects of an openPMD series through a linked list
     * of parents. This method will walk up the parent list until it reaches
     * an object that has no parent, which is the Series object, and flush()-es
     * it.
     * If the Attributable is an Iteration or any object contained in an
     * Iteration, that Iteration will be flushed regardless of its dirty status.
     *
     * @param backendConfig Further backend-specific instructions on how to
     *                      implement this flush call.
     *                      Must be provided in-line, configuration is not read
     *                      from files.
     */
    void seriesFlush(std::string backendConfig = "{}");

    /** Flush the containing Iteration.
     *
     * Writable connects all objects of an openPMD series through a linked list
     * of parents. This method will walk up the parent list to find
     * the containing Iteration.
     * The Iteration will be flushed regardless if it is dirty.
     *
     * @param backendConfig Further backend-specific instructions on how to
     *                      implement this flush call.
     *                      Must be provided in-line, configuration is not read
     *                      from files.
     */
    void iterationFlush(std::string backendConfig = "{}");

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
         * Notice that RecordComponent::SCALAR does not get included in this
         * list.
         */
        std::vector<std::string> group;
        Access access;

        /** Reconstructs a path that can be passed to a Series constructor */
        std::string filePath() const;
        /** Return the path ob the object within the openPMD file */
        std::string openPMDPath() const;
    };

    /**
     * @brief The path to this object within its containing Series.
     *
     * @return A struct informing about the context of this Attributable.
     */
    MyPath myPath() const;

    /**
     * @brief Sets the object dirty to make internal procedures think it has
     *        been modified.
     */
    void touch();

    // clang-format off
OPENPMD_protected
    // clang-format on

    Series retrieveSeries() const;

    /** Returns the corresponding Iteration
     *
     * Return the openPMD::iteration that this Attributable is contained in.
     * This walks up the linked parents until it finds the Iteration object.
     * Throws an error otherwise, e.g., for Series objects.
     * @{
     */
    [[nodiscard]] auto containingIteration() const -> std::pair<
        std::optional<internal::IterationData const *>,
        internal::SeriesData const *>;
    auto containingIteration() -> std::
        pair<std::optional<internal::IterationData *>, internal::SeriesData *>;
    /** @} */

    template <bool flush_entire_series>
    void seriesFlush_impl(internal::FlushParams const &);

    void flushAttributes(internal::FlushParams const &);

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
        return const_cast<AbstractIOHandler *>(
            static_cast<Attributable const *>(this)->IOHandler());
    }
    AbstractIOHandler const *IOHandler() const
    {
        auto &opt = m_attri->m_writable.IOHandler;
        if (!opt || !opt->has_value())
        {
            return nullptr;
        }
        return &*opt->value();
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

    inline void setData(std::shared_ptr<internal::AttributableData> attri)
    {
        m_attri = std::move(attri);
    }

    inline internal::AttributableData &get()
    {
        return *m_attri;
    }
    inline internal::AttributableData const &get() const
    {
        return *m_attri;
    }

    bool dirty() const
    {
        return writable().dirtySelf;
    }
    /** O(1).
     */
    bool dirtyRecursive() const
    {
        return writable().dirtyRecursive;
    }
    void setDirty(bool dirty_in)
    {
        auto &w = writable();
        w.dirtySelf = dirty_in;
        setDirtyRecursive(dirty_in);
    }
    /* Amortized O(1) if dirty_in is true, else O(1).
     *
     * Must be used carefully with `dirty_in == false` since it is assumed that
     * all children are not dirty.
     *
     * Invariant of dirtyRecursive:
     *   this->dirtyRecursive implies parent->dirtyRecursive.
     *
     * Hence:
     *
     * * If dirty_in is true: This needs only go up far enough until a parent is
     *   found that itself is dirtyRecursive.
     * * If dirty_in is false: Only sets `this` to `dirtyRecursive == false`.
     *   The caller must ensure that the invariant holds (e.g. clearing
     *   everything during flushing or reading logic).
     */
    void setDirtyRecursive(bool dirty_in)
    {
        auto &w = writable();
        w.dirtyRecursive = dirty_in;
        if (dirty_in)
        {
            auto current = w.parent;
            while (current && !current->dirtyRecursive)
            {
                current->dirtyRecursive = true;
                current = current->parent;
            }
        }
    }
    bool written() const
    {
        return writable().written;
    }
    enum class EnqueueAsynchronously : bool
    {
        Yes,
        No
    };
    /*
     * setWritten() will take effect immediately.
     * But it might additionally be necessary in some situations to enqueue a
     * SET_WRITTEN task to the backend:
     * A single flush() operation might encompass different Iterations. In
     * file-based Iteration encoding, some objects must be written to every
     * single file, thus their `written` flag must be restored to `false` for
     * each Iteration. When flushing multiple Iterations at once, this must
     * happen as an asynchronous IO task.
     */
    void setWritten(bool val, EnqueueAsynchronously);

private:
    /**
     * @brief Link with parent.
     *
     * @param w The Writable representing the parent.
     */
    virtual void linkHierarchy(Writable &w);
}; // Attributable

// note: we explicitly instantiate Attributable::setAttributeImpl for all T in
// Datatype in Attributable.cpp
template <typename T>
inline bool Attributable::setAttribute(std::string const &key, T value)
{
    auto &attri = get();
    if (IOHandler() &&
        IOHandler()->m_seriesStatus == internal::SeriesStatus::Default &&
        Access::READ_ONLY == IOHandler()->m_frontendAccess)
    {
        auxiliary::OutOfRangeMsg const out_of_range_msg(
            "Attribute", "can not be set (read-only).");
        error::throwNoSuchAttribute(out_of_range_msg(key));
    }

    setDirty(true);
    auto it = attri.m_attributes.lower_bound(key);
    if (it != attri.m_attributes.end() &&
        !attri.m_attributes.key_comp()(key, it->first))
    {
        // key already exists in map, just replace the value
        it->second = Attribute(std::move(value));
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

inline bool
Attributable::setAttribute(std::string const &key, char const value[])
{
    return this->setAttribute(key, std::string(value));
}

template <typename T>
inline T Attributable::readFloatingpoint(std::string const &key) const
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    return getAttribute(key).get<T>();
}

template <typename T>
inline std::vector<T>
Attributable::readVectorFloatingpoint(std::string const &key) const
{
    static_assert(
        std::is_floating_point<T>::value,
        "Type of attribute must be floating point");

    return getAttribute(key).get<std::vector<T> >();
}
} // namespace openPMD
