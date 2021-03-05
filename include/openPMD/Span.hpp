/* Copyright 2021 Franz Poeschel
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

#include "openPMD/RecordComponent.hpp"

namespace openPMD
{
/**
 * @brief Subset of C++20 std::span class template.
 *
 * Any existing member behaves equivalently to those documented here:
 * https://en.cppreference.com/w/cpp/container/span
 */
template< typename T >
class Span
{
    friend class RecordComponent;

private:
    using param_t = Parameter< Operation::GET_BUFFER_VIEW >;
    param_t m_param;
    size_t m_size;
    RecordComponent m_recordComponent;

    Span( param_t param, size_t size, RecordComponent recordComponent )
        : m_param( std::move( param ) )
        , m_size( size )
        , m_recordComponent( std::move( recordComponent ) )
    {
        m_param.update = true;
    }

public:
    size_t size() const
    {
        return m_size;
    }

    T * data()
    {
        if( m_param.out->taskSupportedByBackend )
        {
            // might need to update
            m_recordComponent.IOHandler()->enqueue(
                IOTask( &m_recordComponent, m_param ) );
            m_recordComponent.IOHandler()->flush();
        }
        return static_cast< T * >( m_param.out->ptr );
    }

    T & operator[]( size_t i )
    {
        return data()[ i ];
    }
};
}