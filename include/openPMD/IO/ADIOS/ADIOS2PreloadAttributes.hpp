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
#pragma once

#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2

#include <adios2.h>
#include <map>
#include <optional>
#include <variant>

#include "openPMD/Datatype.hpp"
#include "openPMD/auxiliary/Variant.hpp"

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
 * Class that is responsible for buffering loaded openPMD attributes from
 * ADIOS2.
 *
 * This is used for reading variable-encoded files in random-access mode.
 * Random-access mode in ADIOS2 has the restriction that modifiable attributes
 * can only be recovered in normal non-random-access read mode, so we open the
 * file first in that mode, store all attribute metadata in this class and only
 * then continue in random-access mode.
 *
 */
class PreloadAdiosAttributes
{
public:
    /**
     * Meta information on a buffered attribute.
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

        AttributeLocation(AttributeLocation &&other) noexcept;
        AttributeLocation &operator=(AttributeLocation &&other) noexcept;

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
     * @brief Load attributes from the current step into the buffer.
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

    std::map<std::string, AttributeLocation> const &availableAttributes() const;
};

template <typename T>
struct AttributeWithShapeAndResource : AttributeWithShape<T>
{
    AttributeWithShapeAndResource(AttributeWithShape<T> parent);
    AttributeWithShapeAndResource(
        size_t len_in,
        T const *data_in,
        std::optional<std::vector<T>> resource_in);
    AttributeWithShapeAndResource(adios2::Attribute<T> attr);
    operator bool() const;

private:
    /*
     * Users should still use the API of AttributeWithShape (parent type), we
     * just need somewhere to store the std::vector<T> returned by
     * Attribute<T>::Data(). This field will not be used when using preparsing,
     * because the data pointer will go right into the preparse buffer.
     */
    std::optional<std::vector<T>> resource;
};

struct AdiosAttributes
{
    using RandomAccess_t = std::vector<PreloadAdiosAttributes>;
    struct StreamAccess_t
    {
        /*
         * These are only buffered for performance reasons.
         * IO::AvailableAttributes() returns by value, so we should avoid
         * calling it too often. Instead, store the returned value along with
         * the step, so we know when we need to update again (i.e. when the
         * current step changes).
         */
        size_t m_currentStep = 0;
        std::optional<std::map<std::string, adios2::Params>> m_attributes;
    };

    /*
     * Variant RandomAcces_t has to be initialized explicitly by
     * ADIOS2IOHandlerImpl::readAttributeAllsteps(), so we use StreamAccess_t by
     * default.
     */
    std::variant<RandomAccess_t, StreamAccess_t> m_data = StreamAccess_t{};

    /*
     * Needs to be this somewhat ugly API since the AvailableAttributes map has
     * a different type depending if we use preparsing or not. If we don't use
     * preparsing, we just use the returned std::map<std::string,
     * adios2::Params> from IO::AvailableAttributes(). Otherwise, we use the
     * std::map<std::string, AttributeLocation> map from the
     * PreloadAdiosAttributes class. The functor f will be called either with
     * the one or the other, so needs to be written such that the mapped-to type
     * does not matter.
     */
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
