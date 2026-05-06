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

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace openPMD
{
class CustomHierarchy;
namespace internal
{
    using CustomHierarchyData = ContainerData<Attributable>;
} // namespace internal

class CustomHierarchy;

/*
 * This is its own class, so the return value of asContainerOf() is also
 * convsersible again.
 */
template <typename MappedType>
class ConvertibleContainer : public Container<MappedType>
{
    template <typename>
    friend class ConversibleContainer;
    friend class CustomHierarchy;

protected:
    using Container_t = Container<MappedType>;
    using Data_t = internal::ContainerData<MappedType>;
    static_assert(
        std::is_base_of_v<typename Container_t::ContainerData, Data_t>);

    using Container_t::Container_t;

private:
    explicit ConvertibleContainer() = default;

public:
    template <typename TargetType>
    auto asContainerOf() -> ConvertibleContainer<TargetType>
    {
        if constexpr (
            std::is_same_v<TargetType, CustomHierarchy> ||
            std::is_same_v<TargetType, Mesh> ||
            std::is_same_v<TargetType, ParticleSpecies> ||
            std::is_same_v<TargetType, RecordComponent>)
        {
            // TODO: If Mesh or ParticleSpecies, create Container on the fly by
            // evaluating the meshes/particles path. If RecordComponent, maybe
            // use dynamic casting to check if children are datasets?
            throw std::runtime_error("UNIMPLEMENTED");
        }
        else
        {
            static_assert(
                auxiliary::dependent_false_v<TargetType>,
                "[CustomHierarchy::asContainerOf] Type parameter must be "
                "one of: CustomHierarchy, RecordComponent, Mesh, "
                "ParticleSpecies.");
        }
    }
};

namespace traits
{
    template <>
    struct DeferredInitPolicy<Container<CustomHierarchy>>
    {
        template <typename Container_const_or_not>
        static void call(Container_const_or_not &);
    };
} // namespace traits

// TODO: Use Container<Attributable> internally, but otherwise override members
// such that we have:
//
// operator[](key) -> CustomHierarchy
//
// Or find a better solution for having this automatically..
class CustomHierarchy : public ConvertibleContainer<CustomHierarchy>
{
    friend class Iteration;
    friend class Container<CustomHierarchy>;
    friend class Attributable;
    friend struct traits::DeferredInitPolicy<Container<CustomHierarchy>>;

private:
    using Parent_t = ConvertibleContainer<CustomHierarchy>;
    using Container_t = typename Parent_t::Container_t;
    using Data_t = typename Parent_t::Data_t;

protected:
    CustomHierarchy();
    CustomHierarchy(NoInit);
    CustomHierarchy(std::shared_ptr<internal::SharedAttributableData> other);
    CustomHierarchy(Attributable const &other);

    void read();
    void read(std::vector<std::string> &currentPath);

    void flush_internal(
        internal::FlushParams const &, std::vector<std::string> currentPath);
    void flush(std::string const &path, internal::FlushParams const &) override;

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
};
} // namespace openPMD
