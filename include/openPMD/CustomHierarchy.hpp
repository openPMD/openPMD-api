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
    struct CustomHierarchyData : ContainerData<CustomHierarchy>
    {
        bool has_been_read = false;
    };
} // namespace internal

class CustomHierarchy;

class CustomDataset : public RecordComponent
{
    template <typename>
    friend class ConvertibleContainer;
    friend class CustomHierarchy;
    friend class Attributable;
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
    using Data_t = internal::ContainerData<MappedType>;
    static_assert(
        std::is_base_of_v<typename Container_t::ContainerData, Data_t>);

    using Container_t::Container_t;

private:
    explicit ConvertibleContainer() = default;
    ConvertibleContainer(Attributable::NoInit)
        : Container_t(Attributable::NoInit{})
    {}

public:
    auto isDataset() -> bool;
    auto asDataset() -> CustomDataset;

    auto datasets() -> ConvertibleContainer<CustomDataset>;
    auto subgroups() -> CustomHierarchy;
    // TODO also: asContainerOf()
};

namespace traits
{
    template <>
    struct ElementAccessPolicy<CustomHierarchy>
    {
        static void call(CustomHierarchy &);
        static void call(CustomHierarchy const &);

    private:
        static void syncContainers(
            CustomHierarchy const &,
            internal::object_type::GroupMetaData const &);
    };

    namespace detail
    {
        template <typename Container, typename Iterator>
        void emplace_object_as_customely_managed(Container &cont, Iterator &it)
        {
            auto cast_to_custom_hierarchy = [&it]() {
                using MappedType = std::remove_cv_t<
                    std::remove_reference_t<decltype(it->second)>>;
                if constexpr (std::is_same_v<MappedType, CustomHierarchy>)
                {
                    return it->second;
                }
                else
                {
                    return it->second.customHierarchies();
                }
            };
            auto &writable = it->second.writable();

            // These should be different
            auto child_shared_data = &it->second.Attributable::get();
            auto parent_shared_data = &cont.Attributable::get();
            if (child_shared_data == parent_shared_data)
            {
                throw std::runtime_error(
                    "Trying to emplace object as its own child");
            }

            // These might be different, but might also be the same
            //
            // For an explanation, ref. the documentation of
            // Writable::attributable: This is a pointer back to the first
            // created Attributable instance linking this Writable. There might
            // be multiple Attributable objects linking the same backend
            // Writable object when opening multiple "views" on the same backend
            // object, e.g. when a scalar Record is at the same time a
            // RecordComponent, or when reopening an object as a
            // CustomHierarchy.
            //
            // Since CustomHierarchy performs no memory management by default,
            // we must ensure that the backpointer in Writable::attributable
            // remains valid when the frontend instance pointed by
            // Writable::attributable *is* the CustomHierarchy instance (happens
            // when it is the first frontend object created for that backend
            // object).
            auto backpointer = writable.attributable;
            auto emplaced_pointer = it->second.m_attri.get();
            auto *group_metadata =
                (**cont.m_attri).m_writable.objectType.requireGroup();
            if (backpointer == emplaced_pointer)
            {
                group_metadata
                    ->m_children_managed_as_custom_hierarchy[it->first] =
                    // NO move!! The iterator must stay alive
                    std::make_shared<CustomHierarchy>(cast_to_custom_hierarchy());
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
            detail::emplace_object_as_customely_managed<Container, Iterator>(
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
            detail::emplace_object_as_customely_managed<Container, Iterator>(
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
    CustomHierarchy();

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
    OPENPMDAPI_EXPORT void
    visitHierarchyImpl(HierarchyVisitor &v, bool recursive) override;
};
} // namespace openPMD
