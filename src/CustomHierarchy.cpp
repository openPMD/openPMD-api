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

#include "openPMD/CustomHierarchy.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/auxiliary/StringManip.hpp"

#include "openPMD/Dataset.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/Writable.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <string>

namespace openPMD
{
namespace internal
{
    MeshesParticlesPath::MeshesParticlesPath(
        std::optional<std::string> meshesPath_in,
        std::optional<std::string> particlesPath_in)
        : meshesPath{std::move(meshesPath_in)}
        , particlesPath{std::move(particlesPath_in)}
    {
        if (meshesPath.has_value())
        {
            meshesPath = auxiliary::removeSlashes(*meshesPath);
        }
        if (particlesPath.has_value())
        {
            particlesPath = auxiliary::removeSlashes(*particlesPath);
        }
    }

    ContainedType MeshesParticlesPath::determineType(
        std::vector<std::string> const &path, std::string const &name) const
    {
        if (isMesh(path, name))
        {
            return ContainedType::Mesh;
        }
        else if (isParticle(path, name))
        {
            return ContainedType::Particle;
        }
        else
        {
            return ContainedType::Group;
        }
    }

    bool MeshesParticlesPath::isParticle(
        std::vector<std::string> const &path, std::string const &name) const
    {
        (void)path;
        return particlesPath.has_value() ? name == *particlesPath : false;
    }
    bool MeshesParticlesPath::isMesh(
        std::vector<std::string> const &path, std::string const &name) const
    {
        (void)path;
        return meshesPath.has_value() ? name == *meshesPath : false;
    }
} // namespace internal

CustomHierarchy::CustomHierarchy()
{
    meshes.writable().ownKeyWithinParent = "meshes";
    particles.writable().ownKeyWithinParent = "particles";
}
CustomHierarchy::CustomHierarchy(NoInit) : Container_t(NoInit())
{}

void CustomHierarchy::readMeshes(std::string const &meshesPath)
{
    Parameter<Operation::OPEN_PATH> pOpen;
    Parameter<Operation::LIST_PATHS> pList;

    pOpen.path = meshesPath;
    IOHandler()->enqueue(IOTask(&meshes, pOpen));

    meshes.readAttributes(ReadMode::FullyReread);

    internal::EraseStaleEntries<decltype(meshes)> map{meshes};

    /* obtain all non-scalar meshes */
    IOHandler()->enqueue(IOTask(&meshes, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    Parameter<Operation::LIST_ATTS> aList;
    for (auto const &mesh_name : *pList.paths)
    {
        Mesh &m = map[mesh_name];
        pOpen.path = mesh_name;
        aList.attributes->clear();
        IOHandler()->enqueue(IOTask(&m, pOpen));
        IOHandler()->enqueue(IOTask(&m, aList));
        IOHandler()->flush(internal::defaultFlushParams);

        auto att_begin = aList.attributes->begin();
        auto att_end = aList.attributes->end();
        auto value = std::find(att_begin, att_end, "value");
        auto shape = std::find(att_begin, att_end, "shape");
        if (value != att_end && shape != att_end)
        {
            MeshRecordComponent &mrc = m;
            IOHandler()->enqueue(IOTask(&mrc, pOpen));
            IOHandler()->flush(internal::defaultFlushParams);
            mrc.get().m_isConstant = true;
        }
        try
        {
            m.read();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read mesh with name '" << mesh_name
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
            map.forget(mesh_name);
        }
    }

    /* obtain all scalar meshes */
    Parameter<Operation::LIST_DATASETS> dList;
    IOHandler()->enqueue(IOTask(&meshes, dList));
    IOHandler()->flush(internal::defaultFlushParams);

    Parameter<Operation::OPEN_DATASET> dOpen;
    for (auto const &mesh_name : *dList.datasets)
    {
        Mesh &m = map[mesh_name];
        dOpen.name = mesh_name;
        IOHandler()->enqueue(IOTask(&m, dOpen));
        IOHandler()->flush(internal::defaultFlushParams);
        MeshRecordComponent &mrc = m;
        IOHandler()->enqueue(IOTask(&mrc, dOpen));
        IOHandler()->flush(internal::defaultFlushParams);
        mrc.setWritten(false, Attributable::EnqueueAsynchronously::No);
        mrc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        mrc.setWritten(true, Attributable::EnqueueAsynchronously::No);
        try
        {
            m.read();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read mesh with name '" << mesh_name
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
            map.forget(mesh_name);
        }
    }
}

void CustomHierarchy::readParticles(std::string const &particlesPath)
{
    Parameter<Operation::OPEN_PATH> pOpen;
    Parameter<Operation::LIST_PATHS> pList;

    pOpen.path = particlesPath;
    IOHandler()->enqueue(IOTask(&particles, pOpen));

    particles.readAttributes(ReadMode::FullyReread);

    /* obtain all particle species */
    pList.paths->clear();
    IOHandler()->enqueue(IOTask(&particles, pList));
    IOHandler()->flush(internal::defaultFlushParams);

    internal::EraseStaleEntries<decltype(particles)> map{particles};
    for (auto const &species_name : *pList.paths)
    {
        ParticleSpecies &p = map[species_name];
        pOpen.path = species_name;
        IOHandler()->enqueue(IOTask(&p, pOpen));
        IOHandler()->flush(internal::defaultFlushParams);
        try
        {
            p.read();
        }
        catch (error::ReadError const &err)
        {
            std::cerr << "Cannot read particle species with name '"
                      << species_name
                      << "' and will skip it due to read error:\n"
                      << err.what() << std::endl;
            map.forget(species_name);
        }
    }
}

void CustomHierarchy::read(internal::MeshesParticlesPath const &mpp)
{
    std::vector<std::string> currentPath;
    read(mpp, currentPath);
}

void CustomHierarchy::read(
    internal::MeshesParticlesPath const &mpp,
    std::vector<std::string> &currentPath)
{
    /*
     * Convention for CustomHierarchy::flush and CustomHierarchy::read:
     * Path is created/opened already at entry point of method, method needs
     * to create/open path for contained subpaths.
     */

    Parameter<Operation::LIST_PATHS> pList;
    IOHandler()->enqueue(IOTask(this, pList));

    Attributable::readAttributes(ReadMode::FullyReread);
    Parameter<Operation::LIST_DATASETS> dList;
    IOHandler()->enqueue(IOTask(this, dList));
    IOHandler()->flush(internal::defaultFlushParams);

    for (auto const &path : *pList.paths)
    {
        switch (mpp.determineType(currentPath, path))
        {
        case internal::ContainedType::Group: {
            Parameter<Operation::OPEN_PATH> pOpen;
            pOpen.path = path;
            auto &subpath = this->operator[](path);
            IOHandler()->enqueue(IOTask(&subpath, pOpen));
            currentPath.emplace_back(path);
            try
            {
                subpath.read(mpp, currentPath);
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read subgroup '" << path << "' at path '"
                          << myPath().openPMDPath()
                          << "' and will skip it due to read error:\n"
                          << err.what() << std::endl;
                container().erase(path);
            }
            currentPath.pop_back();
            break;
        }
        case internal::ContainedType::Mesh: {
            try
            {
                readMeshes(*mpp.meshesPath);
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read meshes at path '"
                          << myPath().openPMDPath()
                          << "' and will skip them due to read error:\n"
                          << err.what() << std::endl;
                meshes = {};
                meshes.dirty() = false;
            }
            break;
        }
        case internal::ContainedType::Particle: {
            try
            {
                readParticles(*mpp.particlesPath);
            }
            catch (error::ReadError const &err)
            {
                std::cerr << "Cannot read particles at path '"
                          << myPath().openPMDPath()
                          << "' and will skip them due to read error:\n"
                          << err.what() << std::endl;
                particles = {};
                particles.dirty() = false;
            }
            break;
        }
        }
    }

    if (!mpp.meshesPath.has_value())
    {
        meshes.dirty() = false;
    }
    if (!mpp.particlesPath.has_value())
    {
        particles.dirty() = false;
    }
}

void CustomHierarchy::flush_internal(
    internal::FlushParams const &flushParams,
    internal::MeshesParticlesPath &mpp,
    std::vector<std::string> currentPath)
{
    /*
     * Convention for CustomHierarchy::flush and CustomHierarchy::read:
     * Path is created/opened already at entry point of method, method needs
     * to create/open path for contained subpaths.
     */

    if (access::write(IOHandler()->m_frontendAccess))
    {
        flushAttributes(flushParams);
    }

    Parameter<Operation::CREATE_PATH> pCreate;
    for (auto &[name, subpath] : *this)
    {
        if (!subpath.written())
        {
            pCreate.path = name;
            IOHandler()->enqueue(IOTask(&subpath, pCreate));
        }
        currentPath.emplace_back(name);
        subpath.flush_internal(flushParams, mpp, currentPath);
        currentPath.pop_back();
    }
    if (!meshes.empty() || mpp.meshesPath.has_value())
    {
        if (!access::readOnly(IOHandler()->m_frontendAccess))
        {
            if (!mpp.meshesPath.has_value())
            {
                mpp.meshesPath = "meshes";
            }
            meshes.flush(mpp.meshesPath.value(), flushParams);
        }
        for (auto &m : meshes)
            m.second.flush(m.first, flushParams);
    }
    else
    {
        meshes.dirty() = false;
    }
    if (!particles.empty() || mpp.particlesPath.has_value())
    {
        if (!access::readOnly(IOHandler()->m_frontendAccess))
        {
            if (!mpp.particlesPath.has_value())
            {
                mpp.particlesPath = "particles";
            }
            particles.flush(mpp.particlesPath.value(), flushParams);
        }
        for (auto &m : particles)
            m.second.flush(m.first, flushParams);
    }
    else
    {
        particles.dirty() = false;
    }
}

void CustomHierarchy::flush(
    std::string const & /* path */, internal::FlushParams const &flushParams)
{
    /*
     * Convention for CustomHierarchy::flush and CustomHierarchy::read:
     * Path is created/opened already at entry point of method, method needs
     * to create/open path for contained subpaths.
     */

    Series s = this->retrieveSeries();
    internal::MeshesParticlesPath mpp(
        s.containsAttribute("meshesPath") ? std::make_optional(s.meshesPath())
                                          : std::nullopt,
        s.containsAttribute("particlesPath")
            ? std::make_optional(s.particlesPath())
            : std::nullopt);
    std::vector<std::string> currentPath;
    flush_internal(flushParams, mpp, currentPath);
    if (mpp.meshesPath.has_value() && !s.containsAttribute("meshesPath"))
    {
        s.setMeshesPath(*mpp.meshesPath);
    }
    if (mpp.particlesPath.has_value() && !s.containsAttribute("particlesPath"))
    {
        s.setParticlesPath(*mpp.particlesPath);
    }
}

void CustomHierarchy::linkHierarchy(Writable &w)
{
    Attributable::linkHierarchy(w);
    meshes.linkHierarchy(this->writable());
    particles.linkHierarchy(this->writable());
}

bool CustomHierarchy::dirtyRecursive() const
{
    if (dirty())
    {
        return true;
    }
    if (particles.dirty() || meshes.dirty())
    {
        return true;
    }
    for (auto const &pair : particles)
    {
        if (pair.second.dirtyRecursive())
        {
            return true;
        }
    }
    for (auto const &pair : meshes)
    {
        if (pair.second.dirtyRecursive())
        {
            return true;
        }
    }
    for (auto const &pair : *this)
    {
        if (pair.second.dirtyRecursive())
        {
            return true;
        }
    }
    return false;
}
} // namespace openPMD
