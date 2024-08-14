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
#include "openPMD/backend/Container.hpp"

#include <memory>
#include <regex>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace openPMD
{
class CustomHierarchy;
namespace internal
{
    enum class ContainedType
    {
        Group,
        Mesh,
        Particle
    };
    struct MeshesParticlesPath
    {
        std::regex meshRegex;
        std::set<std::string> collectNewMeshesPaths;
        std::regex particleRegex;
        std::set<std::string> collectNewParticlesPaths;

        /*
         * These values decide which path will be returned upon use of the
         * shorthand notation s.iterations[0].meshes or .particles.
         *
         */
        std::string m_defaultMeshesPath = "meshes";
        std::string m_defaultParticlesPath = "particles";

        explicit MeshesParticlesPath() = default;
        MeshesParticlesPath(
            std::vector<std::string> const &meshes,
            std::vector<std::string> const &particles);
        MeshesParticlesPath(Series const &);

        [[nodiscard]] ContainedType
        determineType(std::vector<std::string> const &path) const;
        [[nodiscard]] bool
        isParticleContainer(std::vector<std::string> const &path) const;
        [[nodiscard]] bool
        isMeshContainer(std::vector<std::string> const &path) const;
    };

    struct CustomHierarchyData
        : ContainerData<CustomHierarchy>
        , ContainerData<RecordComponent>
        , ContainerData<Mesh>
        , ContainerData<ParticleSpecies>
    {
        explicit CustomHierarchyData();

        void syncAttributables();

#if 0
        inline Container<CustomHierarchy> customHierarchiesWrapped()
        {
            Container<CustomHierarchy> res;
            res.setData(
                {static_cast<ContainerData<CustomHierarchy> *>(this),
                 [](auto const *) {}});
            return res;
        }
#endif
        inline Container<RecordComponent> embeddedDatasetsWrapped()
        {
            Container<RecordComponent> res;
            res.setData(
                {static_cast<ContainerData<RecordComponent> *>(this),
                 [](auto const *) {}});
            return res;
        }
        inline Container<Mesh> embeddedMeshesWrapped()
        {
            Container<Mesh> res;
            res.setData(
                {static_cast<ContainerData<Mesh> *>(this),
                 [](auto const *) {}});
            return res;
        }

        inline Container<ParticleSpecies> embeddedParticlesWrapped()
        {
            Container<ParticleSpecies> res;
            res.setData(
                {static_cast<ContainerData<ParticleSpecies> *>(this),
                 [](auto const *) {}});
            return res;
        }

#if 0
        inline Container<CustomHierarchy>::InternalContainer &
        customHierarchiesInternal()
        {
            return static_cast<ContainerData<CustomHierarchy> *>(this)
                ->m_container;
        }
#endif
        inline Container<RecordComponent>::InternalContainer &
        embeddedDatasetsInternal()
        {
            return static_cast<ContainerData<RecordComponent> *>(this)
                ->m_container;
        }
        inline Container<Mesh>::InternalContainer &embeddedMeshesInternal()
        {
            return static_cast<ContainerData<Mesh> *>(this)->m_container;
        }

        inline Container<ParticleSpecies>::InternalContainer &
        embeddedParticlesInternal()
        {
            return static_cast<ContainerData<ParticleSpecies> *>(this)
                ->m_container;
        }
    };
} // namespace internal

template <typename MappedType>
class ConversibleContainer : public Container<MappedType>
{
    template <typename>
    friend class ConversibleContainer;

protected:
    using Container_t = Container<MappedType>;
    using Data_t = internal::CustomHierarchyData;
    static_assert(
        std::is_base_of_v<typename Container_t::ContainerData, Data_t>);

    ConversibleContainer(Attributable::NoInit)
        : Container_t(Attributable::NoInit{})
    {}

    std::shared_ptr<Data_t> m_customHierarchyData;

    [[nodiscard]] Data_t &get()
    {
        return *m_customHierarchyData;
    }
    [[nodiscard]] Data_t const &get() const
    {
        return *m_customHierarchyData;
    }

    inline void setData(std::shared_ptr<Data_t> data)
    {
        m_customHierarchyData = data;
        Container_t::setData(std::move(data));
    }

public:
    template <typename TargetType>
    auto asContainerOf() -> ConversibleContainer<TargetType>
    {
        if constexpr (
            std::is_same_v<TargetType, CustomHierarchy> ||
            std::is_same_v<TargetType, Mesh> ||
            std::is_same_v<TargetType, ParticleSpecies> ||
            std::is_same_v<TargetType, RecordComponent>)
        {
            ConversibleContainer<TargetType> res(Attributable::NoInit{});
            res.setData(m_customHierarchyData);
            return res;
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

class CustomHierarchy : public ConversibleContainer<CustomHierarchy>
{
    friend class Iteration;
    friend class Container<CustomHierarchy>;

private:
    using Container_t = Container<CustomHierarchy>;
    using Parent_t = ConversibleContainer<CustomHierarchy>;
    using Data_t = typename Parent_t::Data_t;

    using EraseStaleMeshes = internal::EraseStaleEntries<Container<Mesh>>;
    using EraseStaleParticles =
        internal::EraseStaleEntries<Container<ParticleSpecies>>;
    void readNonscalarMesh(EraseStaleMeshes &map, std::string const &name);
    void readScalarMesh(EraseStaleMeshes &map, std::string const &name);
    void readParticleSpecies(EraseStaleParticles &map, std::string const &name);

protected:
    CustomHierarchy();
    CustomHierarchy(NoInit);

    void read(internal::MeshesParticlesPath const &);
    void read(
        internal::MeshesParticlesPath const &,
        std::vector<std::string> &currentPath);

    void flush_internal(
        internal::FlushParams const &,
        internal::MeshesParticlesPath &,
        std::vector<std::string> currentPath);
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
