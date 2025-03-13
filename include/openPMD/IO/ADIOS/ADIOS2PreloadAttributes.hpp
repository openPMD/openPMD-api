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
#pragma once

#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/config.hpp"
#include <optional>
#include <variant>
#if openPMD_HAVE_ADIOS2

#include <adios2.h>
#include <map>

#include "openPMD/Datatype.hpp"

namespace openPMD::detail
{
/**
 * @brief Pointer to an attribute's data along with its shape.
 *
 * @tparam T Underlying attribute data type.
 */
template <typename T>
struct AttributeWithShape
{
    size_t len;
    T const *data;
};

/**
 * Class that is responsible for scheduling and buffering openPMD attribute
 * loads from ADIOS2, if using ADIOS variables to store openPMD attributes.
 *
 * Reasoning: ADIOS variables can be of any shape and size, and ADIOS cannot
 * know which variables to buffer. While it will preload and buffer scalar
 * variables, openPMD also stores vector-type attributes which are not
 * preloaded. Since in Streaming setups, every variable load requires full
 * communication back to the writer, this can quickly become very expensive.
 * Hence, do this manually.
 *
 */
class PreloadAdiosAttributes
{
public:
    /**
     * Internally used struct to store meta information on a buffered
     * attribute. Public for simplicity (helper struct in the
     * implementation uses it).
     */
    struct AttributeLocation
    {
        size_t len;
        size_t offset;
        Datatype dt;
        char *destroy = nullptr;

        AttributeLocation() = delete;
        AttributeLocation(size_t len, size_t offset, Datatype dt);

        AttributeLocation(AttributeLocation const &other) = delete;
        AttributeLocation &operator=(AttributeLocation const &other) = delete;

        AttributeLocation(AttributeLocation &&other);
        AttributeLocation &operator=(AttributeLocation &&other);

        ~AttributeLocation();
    };

private:
    /*
     * Allocate one large buffer instead of hundreds of single heap
     * allocations.
     * This will comply with alignment requirements, since
     * std::allocator<char>::allocate() will call the untyped new operator
     * ::operator new(std::size_t)
     * https://en.cppreference.com/w/cpp/memory/allocator/allocate
     */
    std::vector<char> m_rawBuffer;
    std::map<std::string, AttributeLocation> m_offsets;

public:
    explicit PreloadAdiosAttributes() = default;
    PreloadAdiosAttributes(PreloadAdiosAttributes const &other) = delete;
    PreloadAdiosAttributes &
    operator=(PreloadAdiosAttributes const &other) = delete;

    PreloadAdiosAttributes(PreloadAdiosAttributes &&other) = default;
    PreloadAdiosAttributes &operator=(PreloadAdiosAttributes &&other) = default;

    /**
     * @brief Schedule attributes for preloading.
     *
     * This will invalidate all previously buffered attributes.
     * This will *not* flush the scheduled loads. This way, attributes can
     * be loaded along with the next adios2::Engine flush.
     *
     * @param IO
     */
    void preloadAttributes(adios2::IO &IO);

    /**
     * @brief Get an attribute that has been buffered previously.
     *
     * @tparam T The underlying primitive datatype of the attribute.
     *      Will fail if the type found in ADIOS does not match.
     * @param name The full name of the attribute.
     * @return Pointer to the buffered attribute along with information on
     *      the attribute's shape. Valid only until any non-const member
     *      of PreloadAdiosAttributes is called.
     */
    template <typename T>
    AttributeWithShape<T> getAttribute(std::string const &name) const;

    Datatype attributeType(std::string const &name) const;

    std::map<std::string, AttributeLocation> const &
    availableAttributes() const &
    {
        return m_offsets;
    }
};

template <typename T>
struct AttributeWithShapeAndResource : AttributeWithShape<T>
{
    AttributeWithShapeAndResource(AttributeWithShape<T> parent)
        : AttributeWithShape<T>(std::move(parent))
    {}
    AttributeWithShapeAndResource(
        size_t len_in,
        T const *data_in,
        std::optional<std::vector<T>> resource_in)
        : AttributeWithShape<T>{len_in, data_in}
        , resource{std::move(resource_in)}
    {}
    explicit AttributeWithShapeAndResource() : AttributeWithShape<T>(0, nullptr)
    {}
    AttributeWithShapeAndResource(adios2::Attribute<T> attr)
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
    operator bool() const
    {
        return this->data;
    }
    std::optional<std::vector<T>> resource;
};

struct AdiosAttributes
{
    using RandomAccess_t = std::vector<PreloadAdiosAttributes>;
    struct StreamAccess_t
    {
        size_t m_currentStep = 0;
        std::optional<std::map<std::string, adios2::Params>> m_attributes;
    };

    std::variant<RandomAccess_t, StreamAccess_t> m_data = StreamAccess_t{};

    template <typename Functor>
    auto withAvailableAttributes(size_t step, adios2::IO &IO, Functor &&f)
        -> decltype(std::forward<Functor>(f)(
            std::declval<std::map<std::string, adios2::Params> &>()))
    {
        using ret_t = decltype(std::forward<Functor>(f)(
            std::declval<std::map<std::string, adios2::Params> &>()));
        return std::visit(
            auxiliary::overloaded{
                [step, &f](RandomAccess_t &ra) -> ret_t {
                    auto &attribute_data = ra.at(step);
                    return std::forward<Functor>(f)(
                        attribute_data.availableAttributes());
                },
                [step, &f, &IO](StreamAccess_t &sa) -> ret_t {
                    if (!sa.m_attributes.has_value() ||
                        sa.m_currentStep != step)
                    {
                        sa = StreamAccess_t{step, IO.AvailableAttributes()};
                    }
                    return std::forward<Functor>(f)(*sa.m_attributes);
                }},
            m_data);
    }

    template <typename T>
    AttributeWithShapeAndResource<T>
    getAttribute(size_t step, adios2::IO &IO, std::string const &name) const;
};
} // namespace openPMD::detail

#endif // openPMD_HAVE_ADIOS2
