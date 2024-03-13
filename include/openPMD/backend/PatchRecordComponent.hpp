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

#include "openPMD/Error.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/auxiliary/ShareRawInternal.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"

#include <memory>
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

/**
 * @todo add support for constant patch record components
 */
class PatchRecordComponent : public RecordComponent
{
    template <typename T, typename T_key, typename T_container>
    friend class Container;
    template <typename>
    friend class BaseRecord;
    template <typename, typename>
    friend class internal::BaseRecordData;
    friend class ParticlePatches;
    friend class PatchRecord;
    friend class ParticleSpecies;

public:
    /**
     * @brief Avoid object slicing when using a Record as a scalar Record
     *        Component.
     *
     * It's still preferred to directly use the Record, or alternatively a
     * Record-Component-type reference to a Record.
     */
    PatchRecordComponent(BaseRecord<PatchRecordComponent> const &);

    PatchRecordComponent &setUnitSI(double);

    PatchRecordComponent &resetDataset(Dataset) override;

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

    template <typename T>
    void store(T);

    // clang-format off
OPENPMD_private
    // clang-format on

    using RecordComponent::flush;

    // clang-format off
OPENPMD_protected
    // clang-format on

    PatchRecordComponent();
    PatchRecordComponent(NoInit);
}; // PatchRecordComponent

template <typename T>
inline std::shared_ptr<T> PatchRecordComponent::load()
{
    uint64_t numPoints = getExtent()[0];
#if (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION < 11000) ||                   \
    (defined(__apple_build_version__) && __clang_major__ < 14)
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
    rc.push_chunk(IOTask(this, dRead));
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
    rc.push_chunk(IOTask(this, std::move(dWrite)));
}

template <typename T>
inline void PatchRecordComponent::store(T data)
{
    Datatype dtype = determineDatatype<T>();
    if (dtype != getDatatype())
    {
        std::ostringstream oss;
        oss << "Datatypes of patch data (" << dtype << ") and dataset ("
            << getDatatype() << ") do not match.";
        throw std::runtime_error(oss.str());
    }

    if (!joinedDimension().has_value())
    {
        throw error::WrongAPIUsage(
            "[PatchRecordComponent::store] API call without explicit "
            "specification of index only allowed when a joined dimension is "
            "specified.");
    }

    Parameter<Operation::WRITE_DATASET> dWrite;
    dWrite.offset = {};
    dWrite.extent = {1};
    dWrite.dtype = dtype;
    dWrite.data = std::make_shared<T>(data);
    auto &rc = get();
    rc.push_chunk(IOTask(this, std::move(dWrite)));
}
} // namespace openPMD
