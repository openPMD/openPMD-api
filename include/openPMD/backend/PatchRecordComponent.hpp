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

#include "openPMD/auxiliary/ShareRawInternal.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"

#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

// expose private and protected members for invasive testing
#ifndef OPENPMD_private
#define OPENPMD_private private:
#endif

namespace openPMD
{
namespace internal
{
    class PatchRecordComponentData : public BaseRecordComponentData
    {
    public:
        /**
         * Chunk reading/writing requests on the contained dataset.
         */
        std::queue<IOTask> m_chunks;

        PatchRecordComponentData(PatchRecordComponentData const &) = delete;
        PatchRecordComponentData(PatchRecordComponentData &&) = delete;

        PatchRecordComponentData &
        operator=(PatchRecordComponentData const &) = delete;
        PatchRecordComponentData &
        operator=(PatchRecordComponentData &&) = delete;

        PatchRecordComponentData();
    };
} // namespace internal

/**
 * @todo add support for constant patch record components
 */
class PatchRecordComponent : public BaseRecordComponent
{
    template <typename T, typename T_key, typename T_container>
    friend class Container;
    template <typename>
    friend class BaseRecord;
    friend class ParticlePatches;
    friend class PatchRecord;
    friend class ParticleSpecies;
    friend class internal::PatchRecordComponentData;

public:
    PatchRecordComponent &setUnitSI(double);

    PatchRecordComponent &resetDataset(Dataset);

    uint8_t getDimensionality() const;
    Extent getExtent() const;

    template <typename T>
    std::shared_ptr<T> load();

    template <typename T>
    void load(std::shared_ptr<T>);

    template <typename T>
    void load(std::shared_ptr<T[]>);

    template <typename T>
    void loadRaw(T *);

    template <typename T>
    void store(uint64_t idx, T);

    // clang-format off
OPENPMD_private
    // clang-format on

    void flush(std::string const &, internal::FlushParams const &);
    void read();

    /**
     * @brief Check recursively whether this RecordComponent is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool dirtyRecursive() const;

    std::shared_ptr<internal::PatchRecordComponentData>
        m_patchRecordComponentData{new internal::PatchRecordComponentData()};

    PatchRecordComponent();

    // clang-format off
OPENPMD_protected
    // clang-format on

    PatchRecordComponent(std::shared_ptr<internal::PatchRecordComponentData>);

    inline internal::PatchRecordComponentData const &get() const
    {
        return *m_patchRecordComponentData;
    }

    inline internal::PatchRecordComponentData &get()
    {
        return *m_patchRecordComponentData;
    }

    inline void
    setData(std::shared_ptr<internal::PatchRecordComponentData> data)
    {
        m_patchRecordComponentData = std::move(data);
        BaseRecordComponent::setData(m_patchRecordComponentData);
    }
}; // PatchRecordComponent

template <typename T>
inline std::shared_ptr<T> PatchRecordComponent::load()
{
    uint64_t numPoints = getExtent()[0];
#if defined(__clang_major__) && __clang_major__ < 7
    auto newData =
        std::shared_ptr<T>(new T[numPoints], [](T *p) { delete[] p; });
    load(newData);
    return newData;
#else
    auto newData = std::shared_ptr<T[]>(new T[numPoints]);
    load(newData);
    return std::static_pointer_cast<T>(std::move(newData));
#endif
}

template <typename T>
inline void PatchRecordComponent::load(std::shared_ptr<T> data)
{
    Datatype dtype = determineDatatype<T>();
    if (dtype != getDatatype())
        throw std::runtime_error(
            "Type conversion during particle patch loading not yet "
            "implemented");

    if (!data)
        throw std::runtime_error(
            "Unallocated pointer passed during ParticlePatch loading.");

    uint64_t numPoints = getExtent()[0];

    //! @todo add support for constant patch record components

    Parameter<Operation::READ_DATASET> dRead;
    dRead.offset = {0};
    dRead.extent = {numPoints};
    dRead.dtype = getDatatype();
    dRead.data = std::static_pointer_cast<void>(data);
    auto &rc = get();
    rc.m_chunks.push(IOTask(this, dRead));
}

template <typename T>
inline void PatchRecordComponent::load(std::shared_ptr<T[]> data)
{
    load(std::static_pointer_cast<T>(std::move(data)));
}

template <typename T>
inline void PatchRecordComponent::loadRaw(T *data)
{
    load<T>(auxiliary::shareRaw(data));
}

template <typename T>
inline void PatchRecordComponent::store(uint64_t idx, T data)
{
    Datatype dtype = determineDatatype<T>();
    if (dtype != getDatatype())
    {
        std::ostringstream oss;
        oss << "Datatypes of patch data (" << dtype << ") and dataset ("
            << getDatatype() << ") do not match.";
        throw std::runtime_error(oss.str());
    }

    Extent dse = getExtent();
    if (dse[0] - 1u < idx)
        throw std::runtime_error(
            "Index does not reside inside patch (no. patches: " +
            std::to_string(dse[0]) + " - index: " + std::to_string(idx) + ")");

    Parameter<Operation::WRITE_DATASET> dWrite;
    dWrite.offset = {idx};
    dWrite.extent = {1};
    dWrite.dtype = dtype;
    dWrite.data = std::make_shared<T>(data);
    auto &rc = get();
    rc.m_chunks.push(IOTask(this, std::move(dWrite)));
}
} // namespace openPMD
