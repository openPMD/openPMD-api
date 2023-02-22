/* Copyright 2017-2021 Fabian Koller, Axel Huebl and Franz Poeschel
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
#include "openPMD/auxiliary/ShareRaw.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/auxiliary/UniquePtr.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <memory>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#define OPENPMD_protected protected:
#endif

namespace openPMD
{

template <typename T>
class DynamicMemoryView;

class RecordComponent;

namespace internal
{
    class RecordComponentData : public BaseRecordComponentData
    {
    public:
        RecordComponentData();

        RecordComponentData(RecordComponentData const &) = delete;
        RecordComponentData(RecordComponentData &&) = delete;

        RecordComponentData &operator=(RecordComponentData const &) = delete;
        RecordComponentData &operator=(RecordComponentData &&) = delete;

        /**
         * Chunk reading/writing requests on the contained dataset.
         */
        std::queue<IOTask> m_chunks;
        /**
         * Stores the value for constant record components.
         * Ignored otherwise.
         */
        Attribute m_constantValue{-1};
        /**
         * The same std::string that the parent class would pass as parameter to
         * RecordComponent::flush().
         * This is stored only upon RecordComponent::flush() if
         * AbstractIOHandler::flushLevel is set to FlushLevel::SkeletonOnly
         * (for use by the Span<T>-based overload of
         * RecordComponent::storeChunk()).
         * @todo Merge functionality with ownKeyInParent?
         */
        std::string m_name;
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
} // namespace internal

class RecordComponent : public BaseRecordComponent
{
    template <typename T, typename T_key, typename T_container>
    friend class Container;
    friend class Iteration;
    friend class ParticleSpecies;
    template <typename T_elem>
    friend class BaseRecord;
    template <typename T_elem>
    friend class BaseRecordInterface;
    friend class Record;
    friend class Mesh;
    template <typename>
    friend class DynamicMemoryView;
    friend class internal::RecordComponentData;
    friend class MeshRecordComponent;

public:
    enum class Allocation
    {
        USER,
        API,
        AUTO
    }; // Allocation

    RecordComponent &setUnitSI(double);

    /**
     * @brief Declare the dataset's type and extent.
     *
     * Calling this again after flushing will require resizing the dataset.
     * Support for this depends on the backend.
     * Unsupported are:
     * * Changing the datatype.
     * * Shrinking any dimension's extent.
     * * Changing the number of dimensions.
     *
     * Backend support for resizing datasets:
     * * JSON: Supported
     * * ADIOS1: Unsupported
     * * ADIOS2: Supported as of ADIOS2 2.7.0
     * * HDF5: (Currently) unsupported.
     *   Will be probably supported as soon as chunking is supported in HDF5.
     *
     * @return RecordComponent&
     */
    RecordComponent &resetDataset(Dataset);

    uint8_t getDimensionality() const;
    Extent getExtent() const;

    /** Create a dataset with regular extent and constant value
     *
     * In a constant record component, the value for each date in its extent is
     * the same. Implemented by storing only a constant value as meta-data.
     *
     * @tparam T type of the stored value
     * @return A reference to this RecordComponent.
     */
    template <typename T>
    RecordComponent &makeConstant(T);

    /** Create a dataset with zero extent in each dimension.
     *
     * Implemented by creating a constant record component with
     * a dummy value (default constructor of the respective datatype).
     * @param dimensions The number of dimensions. Must be greater than
     *                   zero.
     * @return A reference to this RecordComponent.
     */
    template <typename T>
    RecordComponent &makeEmpty(uint8_t dimensions);

    /**
     * @brief Non-template overload of RecordComponent::makeEmpty().
     * Uses the passed openPMD datatype to determine the template parameter.
     *
     * @param dt The datatype of which to create an empty dataset.
     * @param dimensions The dimensionality of the dataset.
     * @return RecordComponent&
     */
    RecordComponent &makeEmpty(Datatype dt, uint8_t dimensions);

    /** Returns true if this is an empty record component
     *
     * An empty record component has a defined dimensionality but zero extent
     * and no value(s) stored in it.
     *
     * @return true if an empty record component
     */
    bool empty() const;

    /** Load and allocate a chunk of data
     *
     * Set offset to {0u} and extent to {-1u} for full selection.
     *
     * If offset is non-zero and extent is {-1u} the leftover extent in the
     * record component will be selected.
     */
    template <typename T>
    std::shared_ptr<T> loadChunk(Offset = {0u}, Extent = {-1u});

    /** Load a chunk of data into pre-allocated memory.
     *
     * @param data   Preallocated, contiguous buffer, large enough to load the
     *               the requested data into it.
     *               The shared pointer must either own and manage the buffer
     *               or have been created via shareRaw().
     *               If using shareRaw(), it is in the user of this API call's
     *               responsibility to ensure that the lifetime of the buffer
     *               exceeds the next <a
     * href="https://openpmd-api.readthedocs.io/en/latest/usage/workflow.html#deferred-data-api-contract">
     *               flush point</a>.
     *               Optimizations might be implemented based on this
     *               assumption (e.g. skipping the operation if the backend
     *               is the unique owner).
     *               For raw pointers, use loadChunkRaw().
     * @param offset Offset within the dataset. Set to {0u} for full selection.
     * @param extent Extent within the dataset, counted from the offset.
     *               Set to {-1u} for full selection.
     *               If offset is non-zero and extent is {-1u} the leftover
     *               extent in the record component will be selected.
     */
    template <typename T>
    void loadChunk(std::shared_ptr<T> data, Offset offset, Extent extent);

    /** Load a chunk of data into pre-allocated memory, array version.
     *
     * @param data   Preallocated, contiguous buffer, large enough to load the
     *               the requested data into it.
     *               The shared pointer must own and manage the buffer.
     *               Optimizations might be implemented based on this
     *               assumption (e.g. skipping the operation if the backend
     *               is the unique owner).
     *               The array-based overload helps avoid having to manually
     *               specify the delete[] destructor (C++17 feature).
     * @param offset Offset within the dataset. Set to {0u} for full selection.
     * @param extent Extent within the dataset, counted from the offset.
     *               Set to {-1u} for full selection.
     *               If offset is non-zero and extent is {-1u} the leftover
     *               extent in the record component will be selected.
     */
    template <typename T>
    void loadChunk(std::shared_ptr<T[]> data, Offset offset, Extent extent);

    /** Load a chunk of data into pre-allocated memory, raw pointer version.
     *
     * @param data   Preallocated, contiguous buffer, large enough to load the
     *               the requested data into it.
     *               It is in the user of this API call's responsibility to
     *               ensure that the lifetime of the buffer exceeds the next
     *               <a
     * href="https://openpmd-api.readthedocs.io/en/latest/usage/workflow.html#deferred-data-api-contract">
     *               flush point</a>.
     * @param offset Offset within the dataset. Set to {0u} for full selection.
     * @param extent Extent within the dataset, counted from the offset.
     *               Set to {-1u} for full selection.
     *               If offset is non-zero and extent is {-1u} the leftover
     *               extent in the record component will be selected.
     */
    template <typename T>
    void loadChunkRaw(T *data, Offset offset, Extent extent);

    /** Store a chunk of data from a chunk of memory.
     *
     * @param data   Preallocated, contiguous buffer, large enough to read the
     *               the specified data from it.
     *               The shared pointer must either own and manage the buffer
     *               or have been created via shareRaw().
     *               If using shareRaw(), it is in the user of this API call's
     *               responsibility to ensure that the lifetime of the buffer
     *               exceeds the next <a
     * href="https://openpmd-api.readthedocs.io/en/latest/usage/workflow.html#deferred-data-api-contract">
     *               flush point</a>.
     *               Optimizations might be implemented based on this
     *               assumption (e.g. further deferring the operation if the
     *               backend is the unique owner).
     *               For raw pointers, use storeChunkRaw().
     * @param offset Offset within the dataset.
     * @param extent Extent within the dataset, counted from the offset.
     */
    template <typename T>
    void storeChunk(std::shared_ptr<T> data, Offset offset, Extent extent);

    /** Store a chunk of data from a chunk of memory, array version.
     *
     * @param data   Preallocated, contiguous buffer, large enough to read the
     *               the specified data from it.
     *               The array-based overload helps avoid having to manually
     *               specify the delete[] destructor (C++17 feature).
     * @param offset Offset within the dataset.
     * @param extent Extent within the dataset, counted from the offset.
     */
    template <typename T>
    void storeChunk(std::shared_ptr<T[]> data, Offset offset, Extent extent);

    /** Store a chunk of data from a chunk of memory, unique pointer version.
     *
     * @param data   Preallocated, contiguous buffer, large enough to read the
     *               the specified data from it.
     *               The unique pointer must own and manage the buffer.
     *               Optimizations might be implemented based on this
     *               assumption (e.g. further deferring the operation if the
     *               backend is the unique owner).
     *               For raw pointers, use storeChunkRaw().
     * @param offset Offset within the dataset.
     * @param extent Extent within the dataset, counted from the offset.
     */
    template <typename T>
    void storeChunk(UniquePtrWithLambda<T> data, Offset offset, Extent extent);

    /** Store a chunk of data from a chunk of memory, unique pointer version.
     *
     * @param data   Preallocated, contiguous buffer, large enough to read the
     *               the specified data from it.
     *               The unique pointer must own and manage the buffer.
     *               Optimizations might be implemented based on this
     *               assumption (e.g. further deferring the operation if the
     *               backend is the unique owner).
     *               For raw pointers, use storeChunkRaw().
     * @param offset Offset within the dataset.
     * @param extent Extent within the dataset, counted from the offset.
     */
    template <typename T, typename Del>
    void storeChunk(std::unique_ptr<T, Del> data, Offset offset, Extent extent);

    /** Store a chunk of data from a chunk of memory, raw pointer version.
     *
     * @param data   Preallocated, contiguous buffer, large enough to read the
     *               the specified data from it.
     *               It is in the user of this API call's responsibility to
     *               ensure that the lifetime of the buffer exceeds the next
     *               <a
     * href="https://openpmd-api.readthedocs.io/en/latest/usage/workflow.html#deferred-data-api-contract">
     *               flush point</a>.
     * @param offset Offset within the dataset.
     * @param extent Extent within the dataset, counted from the offset.
     */
    template <typename T>
    void storeChunkRaw(T *data, Offset offset, Extent extent);

    /** Store a chunk of data from a contiguous container.
     *
     * @param data   <a
     *               href="https://en.cppreference.com/w/cpp/named_req/ContiguousContainer">
     *               Contiguous container</a>, large enough to read the the
     *               specified data from it. A contiguous container in here is
     *               either a std::vector or a std::array.
     *               It is in the user of this API call's responsibility to
     *               ensure that the lifetime of the container exceeds the next
     *               <a
     * href="https://openpmd-api.readthedocs.io/en/latest/usage/workflow.html#deferred-data-api-contract">
     *               flush point</a>.
     * @param offset Offset within the dataset.
     * @param extent Extent within the dataset, counted from the offset.
     */
    template <typename T_ContiguousContainer>
    typename std::enable_if_t<
        auxiliary::IsContiguousContainer_v<T_ContiguousContainer>>
    storeChunk(
        T_ContiguousContainer &data,
        Offset offset = {0u},
        Extent extent = {-1u});

    /**
     * @brief Overload of storeChunk() that lets the openPMD API allocate
     *        a buffer.
     *
     * This may save memory if the openPMD backend in use is able to provide
     * users a view into its own buffers, avoiding the need to allocate
     * a new buffer.
     *
     * Data can be written into the returned buffer until the next <a
     * href="https://openpmd-api.readthedocs.io/en/latest/usage/workflow.html#deferred-data-api-contract">
     * flush point </a> at which time the data will be read from.
     *
     * In order to provide a view into backend buffers, this call must possibly
     * create files and datasets in the backend, making it MPI-collective.
     * In order to avoid this, calling Series::flush() prior to this is
     * recommended to flush definitions.
     *
     * @param offset Offset within the dataset.
     * @param extent Extent within the dataset, counted from the offset.
     * @param createBuffer If the backend in use has no special support for this
     *        operation, the openPMD API will fall back to creating a buffer,
     *        queuing it for writing and returning a view into that buffer to
     *        the user. The functor createBuffer will be called for this
     *        purpose. It consumes a length parameter of type size_t and should
     *        return a shared_ptr of type T to a buffer at least that length.
     *        In that case, using this API call is equivalent to (1) creating
     *        a shared pointer via createBuffer and (2) then using the regular
     *        storeChunk() API on it.
     *        If the backend supports it, the buffer is not read before the next
     *        flush point and becomes invalid afterwards.
     *
     * @return View into a buffer that can be filled with data.
     */
    template <typename T, typename F>
    DynamicMemoryView<T>
    storeChunk(Offset offset, Extent extent, F &&createBuffer);

    /**
     * Overload of span-based storeChunk() that uses operator new() to create
     * a buffer.
     */
    template <typename T>
    DynamicMemoryView<T> storeChunk(Offset, Extent);

    static constexpr char const *const SCALAR = "\vScalar";

private:
    void flush(std::string const &, internal::FlushParams const &);
    virtual void read();

    /**
     * Internal method to be called by all methods that create an empty dataset.
     *
     * @param d The dataset description. Must have nonzero dimensions.
     * @return Reference to this RecordComponent instance.
     */
    RecordComponent &makeEmpty(Dataset d);

    void storeChunk(
        auxiliary::WriteBuffer buffer, Datatype datatype, Offset o, Extent e);

    /**
     * @brief Check recursively whether this RecordComponent is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool dirtyRecursive() const;

    std::shared_ptr<internal::RecordComponentData> m_recordComponentData{
        new internal::RecordComponentData()};

    RecordComponent();

    // clang-format off
OPENPMD_protected
    // clang-format on

    RecordComponent(std::shared_ptr<internal::RecordComponentData>);

    inline internal::RecordComponentData const &get() const
    {
        return *m_recordComponentData;
    }

    inline internal::RecordComponentData &get()
    {
        return *m_recordComponentData;
    }

    inline void setData(std::shared_ptr<internal::RecordComponentData> data)
    {
        m_recordComponentData = std::move(data);
        BaseRecordComponent::setData(m_recordComponentData);
    }

    void readBase();
}; // RecordComponent

} // namespace openPMD

#include "RecordComponent.tpp"
