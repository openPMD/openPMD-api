/* Copyright 2017-2025 Fabian Koller, Axel Huebl, Franz Poeschel, Luca Fedeli
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

#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

// expose private and protected members for invasive testing
#ifndef OPENPMD_private
#define OPENPMD_private private:
#endif

namespace openPMD
{
namespace test
{
    struct TestHelper;
} // namespace test
class AbstractFilePosition;
class AbstractIOHandler;
struct ADIOS2FilePosition;
template <typename FilePositionType>
class AbstractIOHandlerImplCommon;
template <typename>
class Span;
class Series;
class CustomHierarchy;

namespace traits
{
    template <typename T>
    struct GenerationPolicy;
    template <typename>
    struct ElementAccessPolicy;
} // namespace traits

namespace internal
{
    class SharedAttributableData;
    class AttributableData;
    class SeriesData;
    class ScientificDefaults;
    class BaseRecordComponentData;
    class RecordComponentData;
} // namespace internal

namespace internal::object_type
{
    struct DatasetMetaData
    {
        /**
         * Chunk reading/writing requests on the contained dataset.
         */
        std::queue<IOTask> m_chunks;
        /**
         * The type and extent of the dataset defined by this component.
         */
        std::optional<Dataset> m_dataset;
        /**
         * Stores the value for constant record components.
         * Ignored otherwise.
         */
        Attribute m_constantValue{-1};
        /**
         * True if this is defined as a constant record component as specified
         * in the openPMD standard.
         * If yes, then no heavy-weight dataset is created and the dataset is
         * instead defined via light-weight attributes.
         */
        bool m_isConstant = false;

        /**
         * True if this component is an empty dataset, i.e. its extent is zero
         * in at least one dimension.
         * Treated by the openPMD-api as a special case of constant record
         * components.
         */
        bool m_isEmpty = false;
        /**
         * User has extended the dataset, but the EXTEND task must yet be
         * flushed to the backend
         */
        bool m_hasBeenExtended = false;
    };

    struct GroupMetaData
    {
        // Using shared_ptr<SharedAttributableData> in here because
        // AttributableData is non-copyable and non-movable, but we need
        // movability for map handling
        // Disallowing move in AttributableData is only a measure for code
        // discipline anyway.
        using children_map_t =
            std::map<std::string, std::shared_ptr<SharedAttributableData>>;
        children_map_t m_children;

        // Attributable::customHierarchies() creates objects of type
        // CustomHierarchy ephemerally on the spot. If that object is the first
        // object for its associated SharedAttributableData instance, it must be
        // stored somewhere still, because the first instance is back-referenced
        // by Writable class (TODO: turn that back-reference into a weak_ptr?).
        // Store these objects in the parent to avoid reference cycles.
        // Need shared_ptr because size is not yet known and unique_ptr cannot
        // be managed by std::map.
        using children_object_storage_t =
            std::map<std::string, std::shared_ptr<CustomHierarchy>>;
        children_object_storage_t m_children_managed_as_custom_hierarchy;

        bool phantom = false;
    };
} // namespace internal::object_type

namespace internal
{
    struct ObjectType
        : std::variant<object_type::DatasetMetaData, object_type::GroupMetaData>
    {
        using variant_t = std::
            variant<object_type::DatasetMetaData, object_type::GroupMetaData>;
        using variant_t::variant;

        [[nodiscard]] auto as_base() const -> variant_t const &
        {
            return *this;
        }
        auto as_base() -> variant_t &
        {
            return *this;
        }
        [[nodiscard]] auto isDataset() const -> bool
        {
            return std::holds_alternative<object_type::DatasetMetaData>(
                as_base());
        }
        [[nodiscard]] auto isGroup() const -> bool
        {
            return std::holds_alternative<object_type::GroupMetaData>(
                as_base());
        }
        auto initDataset() -> object_type::DatasetMetaData *
        {
            if (auto res =
                    std::get_if<object_type::DatasetMetaData>(&as_base());
                res)
            {
                return res;
            }
            return &as_base().emplace<object_type::DatasetMetaData>();
        }
        auto initGroup() -> object_type::GroupMetaData *
        {
            if (auto res = std::get_if<object_type::GroupMetaData>(&as_base());
                res)
            {
                return res;
            }
            return &as_base().emplace<object_type::GroupMetaData>();
        }

        auto requireGroup() -> object_type::GroupMetaData *
        {
            if (auto res = std::get_if<object_type::GroupMetaData>(&as_base());
                res)
            {
                return res;
            }
            throw error::Internal(
                "Internal object status: Group type required.");
        }

        template <typename Functor>
        void ifGroup(Functor &&f)
        {
            std::visit(
                auxiliary::overloaded{
                    [&f](object_type::GroupMetaData &object_metadata) {
                        std::forward<Functor>(f)(object_metadata);
                    },
                    [](object_type::DatasetMetaData &) {}},
                as_base());
        }

        template <typename Functor>
        void ifGroup(Functor &&f) const
        {
            std::visit(
                auxiliary::overloaded{
                    [&f](object_type::GroupMetaData const &object_metadata) {
                        std::forward<Functor>(f)(object_metadata);
                    },
                    [](object_type::DatasetMetaData const &) {}},
                as_base());
        }
    };
} // namespace internal
namespace detail
{
    class ADIOS2File;
}

namespace debug
{
    void printDirty(Series const &);
}

/** @brief Layer to mirror structure of logical data and persistent data in
 * file.
 *
 * Hierarchy of objects (datasets, groups, attributes, ...) in openPMD is
 * managed in this class.
 * It also indicates the current synchronization state between logical
 * and persistent data: - whether the object has been created in persistent form
 *                      - whether the logical object has been modified compared
 *                        to last persistent state
 */
class Writable final
{
    friend class internal::SharedAttributableData;
    friend class internal::AttributableData;
    friend class internal::SeriesData;
    friend class Attributable;
    template <typename T_elem>
    friend class BaseRecord;
    template <typename T_elem>
    friend class BaseRecordInterface;
    template <typename T, typename T_key, typename T_container>
    friend class Container;
    friend class Iteration;
    friend class Mesh;
    friend class ParticleSpecies;
    friend class Series;
    friend class Record;
    friend class RecordComponent;
    friend class AbstractIOHandlerImpl;
    friend class ADIOS2IOHandlerImpl;
    friend class detail::ADIOS2File;
    friend class HDF5IOHandlerImpl;
    friend class ParallelHDF5IOHandlerImpl;
    template <typename>
    friend class AbstractIOHandlerImplCommon;
    friend class JSONIOHandlerImpl;
    friend struct test::TestHelper;
    friend std::string concrete_h5_file_position(Writable *);
    friend std::string concrete_bp1_file_position(Writable *);
    template <typename>
    friend class Span;
    friend void debug::printDirty(Series const &);
    friend struct Parameter<Operation::CREATE_DATASET>;
    friend struct Parameter<Operation::OPEN_DATASET>;
    friend class internal::ScientificDefaults;
    template <typename>
    friend class ConvertibleContainer;
    friend class CustomHierarchy;
    template <typename T>
    friend struct traits::GenerationPolicy;
    friend class internal::BaseRecordComponentData;
    friend class internal::RecordComponentData;
    template <typename>
    friend struct traits::ElementAccessPolicy;

private:
    Writable(internal::AttributableData *);

public:
    ~Writable();

    Writable(Writable const &other) = delete;
    Writable(Writable &&other) = delete;
    Writable &operator=(Writable const &other) = delete;
    Writable &operator=(Writable &&other) = delete;

    /** Flush the corresponding Series object
     *
     * Writable connects all objects of an openPMD series through a linked list
     * of parents. This method will walk up the parent list until it reaches
     * an object that has no parent, which is the Series object, and flush()-es
     * it.
     */
    template <bool flush_entire_series>
    void
    seriesFlush(std::string backendConfig = "{}", bool flush_io_handler = true);

    // clang-format off
OPENPMD_private
    // clang-format on

    template <bool flush_entire_series>
    void seriesFlush(internal::FlushParams const &, bool flush_io_handler);
    /*
     * These members need to be shared pointers since distinct instances of
     * Writable may share them.
     */
    std::shared_ptr<AbstractFilePosition> abstractFilePosition = nullptr;
    /*
     * shared_ptr since the IOHandler is shared by multiple Writable instances.
     * optional to make it possible to release the IOHandler, without first
     * having to destroy every single Writable.
     * unique_ptr since AbstractIOHandler is an abstract class.
     */
    std::shared_ptr<std::optional<std::unique_ptr<AbstractIOHandler>>>
        IOHandler = nullptr;
    /*
     * Link to the containing Attributable.
     * If multiple Attributables share the same Writable, then the creating one.
     * (See SharedAttributableData)
     */
    // TODO turn this into a weak pointer
    internal::AttributableData *attributable = nullptr;
    Writable *parent = nullptr;

    /** Tracks if there are unwritten changes for this specific Writable.
     *
     * Manipulate via Attributable::dirty() and Attributable::setDirty().
     */
    bool dirtySelf = true;
    /**
     * Tracks if there are unwritten changes anywhere in the
     * tree whose ancestor this Writable is.
     *
     * Invariant: this->dirtyRecursive implies parent->dirtyRecursive.
     *
     * dirtySelf and dirtyRecursive are separated since that allows specifying
     * that `this` is not dirty, but some child is.
     *
     * Manipulate via Attributable::dirtyRecursive() and
     * Attributable::setDirtyRecursive().
     */
    bool dirtyRecursive = true;
    /**
     * If parent is not null, then this is a key such that:
     * &(*parent)[key] == this
     */
    std::string ownKeyWithinParent;
    /**
     * @brief Whether a Writable has been written to the backend.
     *
     * The class Writable is used to link objects in our (frontend) object model
     * of the openPMD group hierarchy to the backends.
     * The openPMD hierarchy needs to be built by each backend independently
     * from the frontend. This involves the following tasks:
     * * Opening/creating files/groups/datasets
     * * Setting up the path structure in Writable::abstractFilePosition
     *
     * If those tasks have been performed, the flag written is set as true.
     * The interpretation of that is that the backend has been made aware of the
     * Writable and its meaning within the current dataset.
     *
     */
    bool written = false;

    internal::ObjectType objectType = internal::object_type::GroupMetaData{};
};
} // namespace openPMD
