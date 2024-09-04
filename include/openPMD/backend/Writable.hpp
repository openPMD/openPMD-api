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

#include <memory>
#include <string>
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

namespace internal
{
    class AttributableData;
    class SeriesData;
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
    void seriesFlush(std::string backendConfig = "{}");

    // clang-format off
OPENPMD_private
    // clang-format on

    template <bool flush_entire_series>
    void seriesFlush(internal::FlushParams const &);
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
};
} // namespace openPMD
