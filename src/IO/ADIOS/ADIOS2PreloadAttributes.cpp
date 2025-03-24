/* Copyright 2020-2025 Franz Poeschel
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

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <optional>

namespace openPMD::detail
{
namespace
{
    struct GetAlignment
    {
        template <typename T>
        static constexpr size_t call()
        {
            return alignof(T);
        }

        template <unsigned long, typename... Args>
        static constexpr size_t call(Args &&...)
        {
            return alignof(std::max_align_t);
        }
    };

    struct GetSize
    {
        template <typename T>
        static constexpr size_t call()
        {
            return sizeof(T);
        }

        template <unsigned long, typename... Args>
        static constexpr size_t call(Args &&...)
        {
            return 0;
        }
    };

    struct ScheduleLoad
    {
        template <typename T>
        static void call(
            adios2::IO &IO,
            std::string const &name,
            char *buffer,
            PreloadAdiosAttributes::AttributeLocation &location)
        {
            adios2::Attribute<T> attr = IO.InquireAttribute<T>(name);
            if (!attr)
            {
                throw std::runtime_error(
                    "[ADIOS2] Variable not found: " + name);
            }
            /*
             * MSVC does not like placement new of arrays, so we do it
             * in a loop instead.
             * https://developercommunity.visualstudio.com/t/c-placement-new-is-incorrectly-compiled/206439
             */
            T *dest = reinterpret_cast<T *>(buffer);
            for (size_t i = 0; i < location.len; ++i)
            {
                new (dest + i) T();
            }
            location.destroy = buffer;
            auto data = attr.Data();
            std::copy_n(data.begin(), data.size(), dest);
        }

        static constexpr char const *errorMsg = "ADIOS2";
    };

    struct AttributeLen
    {
        template <typename T>
        static size_t call(adios2::IO &IO, std::string const &name)
        {
            auto attr = IO.InquireAttribute<T>(name);
            if (!attr)
            {
                throw std::runtime_error(
                    "[ADIOS2] Variable not found: " + name);
            }
            return attr.Data().size();
        }

        template <unsigned long n, typename... Args>
        static size_t call(Args &&...)
        {
            return {};
        }
    };

    struct AttributeLocationDestroy
    {
        template <typename T>
        static void call(char *ptr, size_t numItems)
        {
            T *destroy = reinterpret_cast<T *>(ptr);
            for (size_t i = 0; i < numItems; ++i)
            {
                destroy[i].~T();
            }
        }

        template <unsigned long n, typename... Args>
        static void call(Args &&...)
        {}
    };
} // namespace

using AttributeLocation = PreloadAdiosAttributes::AttributeLocation;

AttributeLocation::AttributeLocation(
    size_t len_in, size_t offset_in, Datatype dt_in)
    : len(len_in), offset(offset_in), dt(dt_in)
{}

AttributeLocation::AttributeLocation(AttributeLocation &&other) noexcept
    : len{other.len}, offset{other.offset}, dt{other.dt}, destroy{other.destroy}
{
    other.destroy = nullptr;
}

AttributeLocation &
AttributeLocation::operator=(AttributeLocation &&other) noexcept
{
    this->len = other.len;
    this->offset = other.offset;
    this->dt = other.dt;
    this->destroy = other.destroy;
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
        switchAdios2AttributeType<AttributeLocationDestroy>(dt, destroy, len);
    }
}

void PreloadAdiosAttributes::preloadAttributes(adios2::IO &IO)
{
    m_offsets.clear();
    std::map<Datatype, std::vector<std::string>> attributesByType;
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
    for (auto &attribute : IO.AvailableAttributes())
    {
        // this will give us basic types only, no fancy vectors or similar
        Datatype dt = fromADIOS2Type(IO.AttributeType(attribute.first));
        addAttribute(dt, attribute.first);
    }

    // PHASE 2: get offsets for attributes in buffer
    std::map<Datatype, size_t> offsets;
    size_t currentOffset = 0;
    for (auto &pair : attributesByType)
    {
        size_t alignment = switchAdios2AttributeType<GetAlignment>(pair.first);
        size_t size = switchAdios2AttributeType<GetSize>(pair.first);
        // go to next offset with valid alignment
        size_t modulus = currentOffset % alignment;
        if (modulus > 0)
        {
            currentOffset += alignment - modulus;
        }
        for (std::string &name : pair.second)
        {
            size_t elements =
                switchAdios2AttributeType<AttributeLen>(pair.first, IO, name);

            m_offsets.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(std::move(name)),
                std::forward_as_tuple(elements, currentOffset, pair.first));
            currentOffset += elements * size;
        }
    }
    // now, currentOffset is the number of bytes that we need to allocate
    // PHASE 3: allocate new buffer and schedule loads
    m_rawBuffer.resize(currentOffset);
    for (auto &pair : m_offsets)
    {
        switchAdios2AttributeType<ScheduleLoad>(
            pair.second.dt,
            IO,
            pair.first,
            &m_rawBuffer[pair.second.offset],
            pair.second);
    }
}

template <typename T>
AttributeWithShape<T>
PreloadAdiosAttributes::getAttribute(std::string const &name) const
{
    auto it = m_offsets.find(name);
    if (it == m_offsets.end())
    {
        throw std::runtime_error(
            "[ADIOS2] Requested attribute not found: " + name);
    }
    AttributeLocation const &location = it->second;
    Datatype determinedDatatype = determineDatatype<T>();
    if (location.dt != determinedDatatype)
    {
        std::stringstream errorMsg;
        errorMsg << "[ADIOS2] Wrong datatype for attribute: " << name
                 << "(location.dt=" << location.dt
                 << ", T=" << determineDatatype<T>() << ")";
        throw std::runtime_error(errorMsg.str());
    }
    AttributeWithShape<T> res;
    res.len = location.len;
    res.data = reinterpret_cast<T const *>(&m_rawBuffer[location.offset]);
    return res;
}

Datatype PreloadAdiosAttributes::attributeType(std::string const &name) const
{
    auto it = m_offsets.find(name);
    if (it == m_offsets.end())
    {
        return Datatype::UNDEFINED;
    }
    return it->second.dt;
}

std::map<std::string, AttributeLocation> const &
PreloadAdiosAttributes::availableAttributes() const
{
    return m_offsets;
}

template <typename T>
auto AdiosAttributes::getAttribute(
    size_t step, adios2::IO &IO, std::string const &name) const
    -> AttributeWithShapeAndResource<T>
{
    return std::visit(
        auxiliary::overloaded{
            [step, &name](
                RandomAccess_t const &ra) -> AttributeWithShapeAndResource<T> {
                auto &attribute_data = ra.at(step);
                return attribute_data.getAttribute<T>(name);
            },
            [&name,
             &IO](StreamAccess_t const &) -> AttributeWithShapeAndResource<T> {
                auto attr = IO.InquireAttribute<T>(name);
                return {std::move(attr)};
            }},
        m_data);
}

template <typename T>
AttributeWithShapeAndResource<T>::AttributeWithShapeAndResource(
    AttributeWithShape<T> parent)
    : AttributeWithShape<T>(std::move(parent))
{}
template <typename T>
AttributeWithShapeAndResource<T>::AttributeWithShapeAndResource(
    size_t len_in, T const *data_in, std::optional<std::vector<T>> resource_in)
    : AttributeWithShape<T>{len_in, data_in}, resource{std::move(resource_in)}
{}
template <typename T>
AttributeWithShapeAndResource<T>::AttributeWithShapeAndResource(
    adios2::Attribute<T> attr)
{
    if (!attr)
    {
        this->data = nullptr;
        this->len = 0;
        return;
    }
    auto vec = attr.Data();
    this->len = vec.size();
    this->data = vec.data();
    this->resource = std::move(vec);
}
template <typename T>
AttributeWithShapeAndResource<T>::operator bool() const
{
    return this->data;
}

#define OPENPMD_INSTANTIATE_GETATTRIBUTE(type)                                 \
    template AttributeWithShape<type> PreloadAdiosAttributes::getAttribute(    \
        std::string const &name) const;                                        \
    template auto AdiosAttributes::getAttribute(                               \
        size_t step, adios2::IO &IO, std::string const &name) const            \
        -> AttributeWithShapeAndResource<type>;                                \
    template AttributeWithShapeAndResource<                                    \
        type>::AttributeWithShapeAndResource(AttributeWithShape<type> parent); \
    template AttributeWithShapeAndResource<type>::                             \
        AttributeWithShapeAndResource(                                         \
            size_t len_in,                                                     \
            type const                                                         \
                *data_in, /* NOLINTNEXTLINE(bugprone-macro-parentheses)  */    \
            std::optional<std::vector<type>> resource_in);                     \
    template AttributeWithShapeAndResource<                                    \
        type>::AttributeWithShapeAndResource(adios2::Attribute<type> attr);    \
    template AttributeWithShapeAndResource<type>::operator bool() const;
ADIOS2_FOREACH_TYPE_1ARG(OPENPMD_INSTANTIATE_GETATTRIBUTE)
#undef OPENPMD_INSTANTIATE_GETATTRIBUTE
} // namespace openPMD::detail

#endif // openPMD_HAVE_ADIOS2
