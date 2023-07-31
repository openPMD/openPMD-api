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

#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/backend/Container.hpp"

#include <memory>
#include <type_traits>
#include <utility>

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
        std::optional<std::string> meshesPath;
        std::optional<std::string> particlesPath;

        inline std::string requestMeshesPath()
        {
            meshesPath = meshesPath.value_or("meshes");
            return *meshesPath;
        }
        inline std::string requestParticlesPath()
        {
            particlesPath = particlesPath.value_or("particles");
            return *particlesPath;
        }

        explicit MeshesParticlesPath() = default;
        MeshesParticlesPath(
            std::optional<std::string> meshesPath,
            std::optional<std::string> particlesPath);

        [[nodiscard]] ContainedType determineType(
            std::vector<std::string> const &path,
            std::string const &name) const;
        [[nodiscard]] bool isParticle(
            std::vector<std::string> const &path,
            std::string const &name) const;
        [[nodiscard]] bool isMesh(
            std::vector<std::string> const &path,
            std::string const &name) const;
    };

    struct CustomHierarchyData : ContainerData<CustomHierarchy>
    {
        explicit CustomHierarchyData();

        void syncAttributables();

        Container<RecordComponent> m_embeddedDatasets;
        Container<Mesh> m_embeddedMeshes;
        Container<ParticleSpecies> m_embeddedParticles;
    };
} // namespace internal

class CustomHierarchy : public Container<CustomHierarchy>
{
    friend class Iteration;
    friend class Container<CustomHierarchy>;

private:
    using Container_t = Container<CustomHierarchy>;
    using Data_t = internal::CustomHierarchyData;
    static_assert(std::is_base_of_v<Container_t::ContainerData, Data_t>);

    std::shared_ptr<Data_t> m_customHierarchyData;

    [[nodiscard]] Data_t &get()
    {
        return *m_customHierarchyData;
    }
    [[nodiscard]] Data_t const &get() const
    {
        return *m_customHierarchyData;
    }

    using EraseStaleMeshes = internal::EraseStaleEntries<Container<Mesh>>;
    using EraseStaleParticles =
        internal::EraseStaleEntries<Container<ParticleSpecies>>;
    void readNonscalarMesh(EraseStaleMeshes &map, std::string const &name);
    void readScalarMesh(EraseStaleMeshes &map, std::string const &name);
    void readParticleSpecies(EraseStaleParticles &map, std::string const &name);

protected:
    CustomHierarchy();
    CustomHierarchy(NoInit);

    inline void setData(std::shared_ptr<Data_t> data)
    {
        m_customHierarchyData = data;
        Container_t::setData(std::move(data));
    }

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

    /*
     * @brief Check recursively whether this object is dirty.
     *        It is dirty if any attribute or dataset is read from or written to
     *        the backend.
     *
     * @return true If dirty.
     * @return false Otherwise.
     */
    bool dirtyRecursive() const;

public:
    CustomHierarchy(CustomHierarchy const &other) = default;
    CustomHierarchy(CustomHierarchy &&other) = default;

    CustomHierarchy &operator=(CustomHierarchy const &) = default;
    CustomHierarchy &operator=(CustomHierarchy &&) = default;

    mapped_type &operator[](key_type &&key);
    mapped_type &operator[](key_type const &key);

    template <typename ContainedType>
    auto asContainerOf() -> Container<ContainedType> &;

    Container<Mesh> meshes{};
    Container<ParticleSpecies> particles{};

private:
    template <typename KeyType>
    mapped_type &bracketOperatorImpl(KeyType &&);
};
} // namespace openPMD
