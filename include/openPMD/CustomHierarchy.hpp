/* Copyright 2023 Franz Poeschel
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
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/auxiliary/Export.hpp"
#include "openPMD/auxiliary/TypeTraits.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/Writable.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace openPMD
{
class CustomHierarchy;
namespace internal
{
    struct CustomHierarchyData : public AttributableData
    {
        bool has_been_read = false;
    };
} // namespace internal

class CustomDataset : public RecordComponent
{
    template <typename, typename, typename>
    friend class Container;
    template <typename>
    friend class ConvertibleContainer;
    friend class CustomHierarchy;
    friend class Attributable;

private:
    CustomDataset() = default;
};

/*
 * This is its own class, so the return value of asContainerOf() is also
 * convsertible again.
 */
template <typename MappedType>
class ConvertibleContainer : public Container<MappedType>
{
    template <typename>
    friend class ConvertibleContainer;
    friend class CustomHierarchy;

protected:
    using Container_t = Container<MappedType>;

    using Container_t::Container_t;

private:
    explicit ConvertibleContainer() = default;
    ConvertibleContainer(Attributable::NoInit)
        : Container_t(Attributable::NoInit{})
    {}

public:
    [[nodiscard]] auto isDataset() const -> bool;
    auto asDataset() -> CustomDataset;

    auto datasets() -> ConvertibleContainer<CustomDataset>;
    auto subgroups() -> ConvertibleContainer<CustomHierarchy>;
};

namespace traits
{
    template <>
    struct ElementAccessPolicy<CustomHierarchy>
    {
        static void call(CustomHierarchy &);
        static void call(CustomHierarchy const &);
    };

    namespace detail
    {
        template <typename Container, typename Iterator>
        void emplace_object_as_customly_managed(Container &, Iterator &it)
        {
            auto &writable = it->second.writable();

            // If this frontend instance is the first one for its backend
            // object, the reference graph (Writable::attributable -> frontend
            // data -> SharedAttributableData) is rooted at it. The typed cache
            // now owns that instance, so only record that the object was
            // created through the custom hierarchy.
            auto backpointer = writable.attributable;
            auto emplaced_pointer = it->second.m_attri.get();
            if (backpointer == emplaced_pointer)
            {
                (**it->second.m_attri).managed_as_custom_hierarchy = true;
            }
        }
    } // namespace detail

    template <>
    struct GenerationPolicy<CustomHierarchy>
    {
        constexpr static bool is_noop = false;
        template <typename Container, typename Iterator>
        void operator()(Container &cont, Iterator &it)
        {
            detail::emplace_object_as_customly_managed<Container, Iterator>(
                cont, it);
        }
    };

    template <>
    struct GenerationPolicy<CustomDataset>
    {
        constexpr static bool is_noop = false;
        template <typename Container, typename Iterator>
        void operator()(Container &cont, Iterator &it)
        {
            detail::emplace_object_as_customly_managed<Container, Iterator>(
                cont, it);
        }
    };
} // namespace traits

class CustomHierarchy : public ConvertibleContainer<CustomHierarchy>
{
    friend class Iteration;
    friend class Container<CustomHierarchy>;
    friend class Attributable;
    friend struct traits::ElementAccessPolicy<CustomHierarchy>;
    template <typename, typename, typename>
    friend class Container;
    template <typename>
    friend class ConvertibleContainer;
    template <typename>
    friend struct internal::ChildMaterializer;

private:
    using Parent_t = ConvertibleContainer<CustomHierarchy>;
    using Container_t = typename Parent_t::Container_t;
    using Data_t = internal::CustomHierarchyData;

    std::shared_ptr<Data_t> m_customHierarchyData;

    inline void setData(std::shared_ptr<Data_t> customHierarchyData)
    {
        m_customHierarchyData = std::move(customHierarchyData);
        ConvertibleContainer<CustomHierarchy>::setData(m_customHierarchyData);
    }

    inline auto get() -> Data_t &
    {
        return *m_customHierarchyData;
    }

    [[nodiscard]] inline auto get() const -> Data_t const &
    {
        return *m_customHierarchyData;
    }

protected:
    CustomHierarchy();
    CustomHierarchy(NoInit);
    CustomHierarchy(std::shared_ptr<internal::SharedAttributableData> other);
    CustomHierarchy(Attributable const &other);

    void flush_internal(
        internal::FlushParams const &, std::vector<std::string> currentPath);
    void flush(std::string const &path, internal::FlushParams const &);

    /**
     * @brief Link with parent.
     *
     * @param w The Writable representing the parent.
     */
    void linkHierarchy(Writable &w) override;

public:
    CustomHierarchy(CustomHierarchy const &other) = default;
    CustomHierarchy(CustomHierarchy &&other) = default;

    CustomHierarchy &operator=(CustomHierarchy const &) = default;
    CustomHierarchy &operator=(CustomHierarchy &&) = default;

    // TODO should we automatically read upon returning / instantiating a
    // CustomHierarchy object? i.e. upon Attributable::customHierarchy() and
    // CustomHierarchy::operator[]().
    //
    // set max_recursion_depth = 0 for infinite cycling
    // recursion depth includes the current object
    // recursion will not continue expanding into regions that are already known
    // (hence not transitively expand into unknown subregions of known regions)
    auto read(size_t max_recursion_depth = 1) -> CustomHierarchy &;

    void printRecursively();

private:
    void printRecursively(std::string indent);

protected:
    void visitHierarchyImpl(HierarchyVisitor &v, bool recursive) override;
};
namespace internal
{
    template <>
    struct ChildMaterializer<CustomHierarchy>
    {
        static auto materialize(
            std::string const &,
            std::shared_ptr<SharedAttributableData> const &child)
            -> std::optional<CustomHierarchy>
        {
            return CustomHierarchy(child);
        }
    };

    template <>
    struct ChildMaterializer<CustomDataset>
    {
        static auto materialize(
            std::string const &,
            std::shared_ptr<SharedAttributableData> const &child)
            -> std::optional<CustomDataset>
        {
            CustomHierarchy custom{child};
            if (!custom.isDataset())
            {
                return std::nullopt;
            }
            return custom.asDataset();
        }
    };
} // namespace internal
} // namespace openPMD
