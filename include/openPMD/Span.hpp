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

#include <iterator>

namespace openPMD
{
/**
 * @brief Subset of C++20 std::span class template.
 *
 * Any existing member behaves equivalently to those documented here:
 * https://en.cppreference.com/w/cpp/container/span
 */
template <typename T>
class Span
{
    template <typename>
    friend class DynamicMemoryView;

private:
    T *m_ptr;
    size_t m_size;

    Span(T *ptr, size_t size) : m_ptr(ptr), m_size(size)
    {}

public:
    using iterator = T *;
    using reverse_iterator = std::reverse_iterator<iterator>;

    size_t size() const
    {
        return m_size;
    }

    inline T *data() const
    {
        return m_ptr;
    }

    inline T &operator[](size_t i) const
    {
        return data()[i];
    }

    inline iterator begin() const
    {
        return data();
    }
    inline iterator end() const
    {
        return data() + size();
    }
    inline reverse_iterator rbegin() const
    {
        // std::reverse_iterator does the -1 thing automatically
        return reverse_iterator{data() + size()};
    }
    inline reverse_iterator rend() const
    {
        return reverse_iterator{data()};
    }
};

/**
 * @brief A view into a buffer that might be reallocated at some points and
 *      thus has changing base pointers over time.
 *      Reasoning: ADIOS2's span-based Engine::Put() API returns spans whose
 *      base pointers might change after internal reallocations.
 *      Hence, the concrete pointer needs to be acquired right before writing
 *      to it. Otherwise, a use after free might occur.
 */
template <typename T>
class DynamicMemoryView
{
    friend class RecordComponent;

private:
    using param_t = Parameter<Operation::GET_BUFFER_VIEW>;
    param_t m_param;
    size_t m_size;
    RecordComponent m_recordComponent;

    DynamicMemoryView(
        param_t param, size_t size, RecordComponent recordComponent)
        : m_param(std::move(param))
        , m_size(size)
        , m_recordComponent(std::move(recordComponent))
    {
        m_param.update = true;
    }

public:
    /**
     * @brief Acquire the underlying buffer at its current position in memory.
     */
    Span<T> currentBuffer()
    {
        if (m_param.out->backendManagedBuffer)
        {
            // might need to update
            m_recordComponent.IOHandler()->enqueue(
                IOTask(&m_recordComponent, m_param));
            m_recordComponent.IOHandler()->flush(internal::defaultFlushParams);
        }
        return Span<T>{static_cast<T *>(m_param.out->ptr), m_size};
    }
};
} // namespace openPMD
