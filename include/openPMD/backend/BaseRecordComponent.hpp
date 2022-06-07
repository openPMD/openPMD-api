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

#include "openPMD/Dataset.hpp"
#include "openPMD/backend/Attributable.hpp"

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#define OPENPMD_protected protected
#endif

namespace openPMD
{
class BaseRecordComponent : public LegacyAttributable
{
    template <typename T, typename T_key, typename T_container>
    friend class Container;

public:
    ~BaseRecordComponent() = default;

    double unitSI() const;

    BaseRecordComponent &resetDatatype(Datatype);

    Datatype getDatatype() const;

    /** Returns true if this is a constant record component
     *
     * In a constant record component, the value for each date in its extent is
     * the same.
     *
     * @return true if a constant record component
     */
    bool constant() const;

    /**
     * Get data chunks that are available to be loaded from the backend.
     * Note that this is backend-dependent information and the returned
     * information may hence differ between different backends:
     * * The ADIOS backends (versions 1 and 2) will return those chunks that
     *   the writer has originally written.
     * * The JSON backend will reconstruct the chunks by iterating the dataset.
     * * The HDF5 backend will return the whole dataset as one large chunk.
     *   HDF5's notion of chunking is currently ignored.
     *   (https://support.hdfgroup.org/HDF5/doc/Advanced/Chunking/)
     *
     * The results depend solely on the backend and are independent of any
     * openPMD-related information. Note that this call currently does not take
     * into account the openPMD concept of particle patches, which users
     * may additionally wish to use to store user-defined, backend-independent
     * chunking information on particle datasets.
     */
    ChunkTable availableChunks();

    OPENPMD_protected : BaseRecordComponent();

    std::shared_ptr<Dataset> m_dataset;
    std::shared_ptr<bool> m_isConstant;
}; // BaseRecordComponent

namespace detail
{
    /**
     * Functor template to be used in combination with switchType::operator()
     * to set a default value for constant record components via the
     * respective type's default constructor.
     * Used to implement empty datasets in subclasses of BaseRecordComponent
     * (currently RecordComponent).
     * @param T_RecordComponent
     */
    template <typename T_RecordComponent>
    struct DefaultValue
    {
        template <typename T>
        void operator()(T_RecordComponent &rc)
        {
            rc.makeConstant(T());
        }

        template <unsigned n, typename... Args>
        void operator()(Args &&...)
        {
            throw std::runtime_error(
                "makeEmpty: Datatype not supported by openPMD.");
        }
    };
} // namespace detail
} // namespace openPMD
