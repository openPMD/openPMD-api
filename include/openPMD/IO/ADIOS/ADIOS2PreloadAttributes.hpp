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

#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2

#include <adios2.h>
#include <functional>
#include <map>
#include <sstream>
#include <stddef.h>
#include <type_traits>

#include "openPMD/Datatype.hpp"

namespace openPMD
{
namespace detail
{
    /**
     * @brief Pointer to an attribute's data along with its shape.
     *
     * @tparam T Underlying attribute data type.
     */
    template <typename T>
    struct AttributeWithShape
    {
        adios2::Dims shape;
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
            adios2::Dims shape;
            size_t offset;
            Datatype dt;
            char *destroy = nullptr;

            AttributeLocation() = delete;
            AttributeLocation(adios2::Dims shape, size_t offset, Datatype dt);

            AttributeLocation(AttributeLocation const &other) = delete;
            AttributeLocation &
            operator=(AttributeLocation const &other) = delete;

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
        PreloadAdiosAttributes &
        operator=(PreloadAdiosAttributes &&other) = default;

        /**
         * @brief Schedule attributes for preloading.
         *
         * This will invalidate all previously buffered attributes.
         * This will *not* flush the scheduled loads. This way, attributes can
         * be loaded along with the next adios2::Engine flush.
         *
         * @param IO
         * @param engine
         */
        void preloadAttributes(adios2::IO &IO, adios2::Engine &engine);

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
    };

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
        if (std::is_same<T, signed char>::value)
        {
            // workaround: we use Datatype::CHAR to represent ADIOS2 signed char
            // (ADIOS2 does not have chars with unspecified signed-ness
            // anyway)
            determinedDatatype = Datatype::CHAR;
        }
        if (location.dt != determinedDatatype)
        {
            std::stringstream errorMsg;
            errorMsg << "[ADIOS2] Wrong datatype for attribute: " << name
                     << "(location.dt=" << location.dt
                     << ", T=" << determineDatatype<T>() << ")";
            throw std::runtime_error(errorMsg.str());
        }
        AttributeWithShape<T> res;
        res.shape = location.shape;
        res.data = reinterpret_cast<T const *>(&m_rawBuffer[location.offset]);
        return res;
    }
} // namespace detail
} // namespace openPMD

#endif // openPMD_HAVE_ADIOS2
