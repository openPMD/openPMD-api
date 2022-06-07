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
#define OPENPMD_protected protected
#endif

namespace openPMD
{
namespace traits
{
    /** Emulate in the C++17 concept ContiguousContainer
     *
     * Users can implement this trait for a type to signal it can be used as
     * contiguous container.
     *
     * See:
     *   https://en.cppreference.com/w/cpp/named_req/ContiguousContainer
     */
    template <typename T>
    struct IsContiguousContainer
    {
        static constexpr bool value = false;
    };

    template <typename T_Value>
    struct IsContiguousContainer<std::vector<T_Value> >
    {
        static constexpr bool value = true;
    };

    template <typename T_Value, std::size_t N>
    struct IsContiguousContainer<std::array<T_Value, N> >
    {
        static constexpr bool value = true;
    };
} // namespace traits

template <typename T>
class DynamicMemoryView;

class RecordComponent : public BaseRecordComponent
{
    template <typename T, typename T_key, typename T_container>
    friend class Container;
    friend class Iteration;
    friend class ParticleSpecies;
    template <typename T_elem>
    friend class BaseRecord;
    friend class Record;
    friend class Mesh;
    template <typename>
    friend class DynamicMemoryView;

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

    /** Load a chunk of data into pre-allocated memory
     *
     * shared_ptr for data must be pre-allocated, contiguous and large enough
     * for extent
     *
     * Set offset to {0u} and extent to {-1u} for full selection.
     *
     * If offset is non-zero and extent is {-1u} the leftover extent in the
     * record component will be selected.
     */
    template <typename T>
    void loadChunk(std::shared_ptr<T>, Offset, Extent);

    template <typename T>
    void storeChunk(std::shared_ptr<T>, Offset, Extent);

    template <typename T_ContiguousContainer>
    typename std::enable_if<
        traits::IsContiguousContainer<T_ContiguousContainer>::value>::type
    storeChunk(T_ContiguousContainer &, Offset = {0u}, Extent = {-1u});

    /**
     * @brief Overload of storeChunk() that lets the openPMD API allocate
     *        a buffer.
     *
     * This may save memory if the openPMD backend in use is able to provide
     * users a view into its own buffers, avoiding the need to allocate
     * a new buffer.
     *
     * Data can be written into the returned buffer until the next call to
     * Series::flush() at which time the data will be read from.
     *
     * In order to provide a view into backend buffers, this call must possibly
     * create files and datasets in the backend, making it MPI-collective.
     * In order to avoid this, calling Series::flush() prior to this is
     * recommended to flush definitions.
     *
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
    DynamicMemoryView<T> storeChunk(Offset, Extent, F &&createBuffer);

    /**
     * Overload of span-based storeChunk() that uses operator new() to create
     * a buffer.
     */
    template <typename T>
    DynamicMemoryView<T> storeChunk(Offset, Extent);

    static constexpr char const *const SCALAR = "\vScalar";

    virtual ~RecordComponent() = default;

    OPENPMD_protected:
    RecordComponent();

    void readBase();

    std::shared_ptr<std::queue<IOTask> > m_chunks;
    std::shared_ptr<Attribute> m_constantValue;
    std::shared_ptr<bool> m_isEmpty = std::make_shared<bool>(false);
    // User has extended the dataset, but the EXTEND task must yet be flushed
    // to the backend
    std::shared_ptr<bool> m_hasBeenExtended = std::make_shared<bool>(false);

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

    /**
     * @brief Check recursively whether this RecordComponent is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool dirtyRecursive() const;

protected:
    /**
     * The same std::string that the parent class would pass as parameter to
     * RecordComponent::flush().
     * This is stored only upon RecordComponent::flush() if
     * AbstractIOHandler::flushLevel is set to FlushLevel::SkeletonOnly
     * (for use by the Span<T>-based overload of RecordComponent::storeChunk()).
     * @todo Merge functionality with ownKeyInParent?
     */
    std::shared_ptr<std::string> m_name = std::make_shared<std::string>();

}; // RecordComponent
} // namespace openPMD

#include "RecordComponent.tpp"
