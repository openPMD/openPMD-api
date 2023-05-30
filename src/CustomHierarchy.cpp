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

#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Writable.hpp"

#include <map>
#include <string>

namespace openPMD
{
CustomHierarchy::CustomHierarchy()
{
    meshes.writable().ownKeyWithinParent = "meshes";
    particles.writable().ownKeyWithinParent = "particles";
}
CustomHierarchy::CustomHierarchy(NoInit) : Container_t(NoInit())
{}

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
