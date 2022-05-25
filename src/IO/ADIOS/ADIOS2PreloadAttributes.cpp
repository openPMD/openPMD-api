/* Copyright 2020-2021 Franz Poeschel
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

#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2

#include "openPMD/IO/ADIOS/ADIOS2PreloadAttributes.hpp"

#include "openPMD/Datatype.hpp"
#include "openPMD/IO/ADIOS/ADIOS2Auxiliary.hpp"
#include "openPMD/auxiliary/StringManip.hpp"

#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <type_traits>

namespace openPMD
{
namespace detail
{
    namespace
    {
        struct GetAlignment
        {
            template <typename T>
            constexpr size_t operator()() const
            {
                return alignof(T);
            }

            template <unsigned long, typename... Args>
            constexpr size_t operator()(Args &&...) const
            {
                return alignof(std::max_align_t);
            }
        };

        struct GetSize
        {
            template <typename T>
            constexpr size_t operator()() const
            {
                return sizeof(T);
            }

            template <unsigned long, typename... Args>
            constexpr size_t operator()(Args &&...) const
            {
                return 0;
            }
        };

        struct ScheduleLoad
        {
            template <typename T>
            void operator()(
                adios2::IO &IO,
                adios2::Engine &engine,
                std::string const &name,
                char *buffer,
                PreloadAdiosAttributes::AttributeLocation &location)
            {
                adios2::Variable<T> var = IO.InquireVariable<T>(name);
                if (!var)
                {
                    throw std::runtime_error(
                        "[ADIOS2] Variable not found: " + name);
                }
                adios2::Dims const &shape = location.shape;
                adios2::Dims offset(shape.size(), 0);
                if (shape.size() > 0)
                {
                    var.SetSelection({offset, shape});
                }
                T *dest = reinterpret_cast<T *>(buffer);
                size_t numItems = 1;
                for (auto extent : shape)
                {
                    numItems *= extent;
                }
                /*
                 * MSVC does not like placement new of arrays, so we do it
                 * in a loop instead.
                 * https://developercommunity.visualstudio.com/t/c-placement-new-is-incorrectly-compiled/206439
                 */
                for (size_t i = 0; i < numItems; ++i)
                {
                    new (dest + i) T();
                }
                location.destroy = buffer;
                engine.Get(var, dest, adios2::Mode::Deferred);
            }

            std::string errorMsg = "ADIOS2";
        };

        struct VariableShape
        {
            template <typename T>
            adios2::Dims operator()(adios2::IO &IO, std::string const &name)
            {
                auto var = IO.InquireVariable<T>(name);
                if (!var)
                {
                    throw std::runtime_error(
                        "[ADIOS2] Variable not found: " + name);
                }
                return var.Shape();
            }

            template <unsigned long n, typename... Args>
            adios2::Dims operator()(Args &&...)
            {
                return {};
            }
        };

        struct AttributeLocationDestroy
        {
            template <typename T>
            void operator()(char *ptr, size_t numItems)
            {
                T *destroy = reinterpret_cast<T *>(ptr);
                for (size_t i = 0; i < numItems; ++i)
                {
                    destroy[i].~T();
                }
            }

            template <unsigned long n, typename... Args>
            void operator()(Args &&...)
            {}
        };
    } // namespace

    using AttributeLocation = PreloadAdiosAttributes::AttributeLocation;

    AttributeLocation::AttributeLocation(
        adios2::Dims shape_in, size_t offset_in, Datatype dt_in)
        : shape(std::move(shape_in)), offset(offset_in), dt(dt_in)
    {}

    AttributeLocation::AttributeLocation(AttributeLocation &&other)
        : shape{std::move(other.shape)}
        , offset{std::move(other.offset)}
        , dt{std::move(other.dt)}
        , destroy{std::move(other.destroy)}
    {
        other.destroy = nullptr;
    }

    AttributeLocation &AttributeLocation::operator=(AttributeLocation &&other)
    {
        this->shape = std::move(other.shape);
        this->offset = std::move(other.offset);
        this->dt = std::move(other.dt);
        this->destroy = std::move(other.destroy);
        other.destroy = nullptr;
        return *this;
    }

    PreloadAdiosAttributes::AttributeLocation::~AttributeLocation()
    {
        /*
         * If the object has been moved from, this may be empty.
         * Or else, if no custom destructor has been emplaced.
         */
        if (destroy)
        {
            size_t length = 1;
            for (auto ext : shape)
            {
                length *= ext;
            }
            static AttributeLocationDestroy ald;
            switchAdios2AttributeType(dt, ald, destroy, length);
        }
    }

    void PreloadAdiosAttributes::preloadAttributes(
        adios2::IO &IO, adios2::Engine &engine)
    {
        m_offsets.clear();
        std::map<Datatype, std::vector<std::string> > attributesByType;
        auto addAttribute = [&attributesByType](Datatype dt, std::string name) {
            constexpr size_t reserve = 10;
            auto it = attributesByType.find(dt);
            if (it == attributesByType.end())
            {
                it = attributesByType.emplace_hint(
                    it, dt, std::vector<std::string>());
                it->second.reserve(reserve);
            }
            it->second.push_back(std::move(name));
        };
        // PHASE 1: collect names of available attributes by ADIOS datatype
        for (auto &variable : IO.AvailableVariables())
        {
            if (auxiliary::ends_with(variable.first, "/__data__"))
            {
                continue;
            }
            // this will give us basic types only, no fancy vectors or similar
            Datatype dt = fromADIOS2Type(IO.VariableType(variable.first));
            addAttribute(dt, std::move(variable.first));
        }

        // PHASE 2: get offsets for attributes in buffer
        std::map<Datatype, size_t> offsets;
        size_t currentOffset = 0;
        GetAlignment switchAlignment;
        GetSize switchSize;
        VariableShape switchShape;
        for (auto &pair : attributesByType)
        {
            size_t alignment =
                switchAdios2AttributeType(pair.first, switchAlignment);
            size_t size = switchAdios2AttributeType(pair.first, switchSize);
            // go to next offset with valid alignment
            size_t modulus = currentOffset % alignment;
            if (modulus > 0)
            {
                currentOffset += alignment - modulus;
            }
            for (std::string &name : pair.second)
            {
                adios2::Dims shape = switchAdios2AttributeType(
                    pair.first, switchShape, IO, name);
                size_t elements = 1;
                for (auto extent : shape)
                {
                    elements *= extent;
                }
                m_offsets.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(std::move(name)),
                    std::forward_as_tuple(
                        std::move(shape), currentOffset, pair.first));
                currentOffset += elements * size;
            }
        }
        // now, currentOffset is the number of bytes that we need to allocate
        // PHASE 3: allocate new buffer and schedule loads
        m_rawBuffer.resize(currentOffset);
        ScheduleLoad switchSchedule;
        for (auto &pair : m_offsets)
        {
            switchAdios2AttributeType(
                pair.second.dt,
                switchSchedule,
                IO,
                engine,
                pair.first,
                &m_rawBuffer[pair.second.offset],
                pair.second);
        }
    }

    Datatype
    PreloadAdiosAttributes::attributeType(std::string const &name) const
    {
        auto it = m_offsets.find(name);
        if (it == m_offsets.end())
        {
            return Datatype::UNDEFINED;
        }
        return it->second.dt;
    }
} // namespace detail
} // namespace openPMD

#endif // openPMD_HAVE_ADIOS2
