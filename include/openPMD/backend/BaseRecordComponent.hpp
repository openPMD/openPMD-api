/* Copyright 2017-2020 Fabian Koller
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

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/Dataset.hpp"

// expose private and protected members for invasive testing
#ifndef OPENPMD_protected
#   define OPENPMD_protected protected
#endif


namespace openPMD
{
class BaseRecordComponent : public Attributable
{
    template< typename T_elem >
    friend
    class BaseRecord;

    template<
        typename T,
        typename T_key,
        typename T_container
    >
    friend
    class Container;

public:
    ~BaseRecordComponent() override = default;

    double unitSI() const;

    BaseRecordComponent& resetDatatype(Datatype);

    Datatype getDatatype() const;

OPENPMD_protected:
    BaseRecordComponent();

    std::shared_ptr< Dataset > m_dataset;
    std::shared_ptr< bool > m_isConstant;
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
template< typename T_RecordComponent >
struct DefaultValue
{
    template< typename T >
    void
    operator()( T_RecordComponent & rc )
    {
        rc.makeConstant( T() );
    }

    template< unsigned n, typename... Args >
    void
    operator()( Args &&... )
    {
        throw std::runtime_error(
            "makeEmpty: Datatype not supported by openPMD." );
    }
};
} // namespace detail
} // namespace openPMD
