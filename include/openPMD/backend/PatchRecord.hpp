/* Copyright 2017-2019 Fabian Koller
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

#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/backend/BaseRecord.hpp"

#include <unordered_map>
#include <string>


namespace openPMD
{
class PatchRecord : public BaseRecord< PatchRecordComponent >
{
    friend class Container< PatchRecord >;
    friend class ParticleSpecies;
    friend class ParticlePatches;

public:
    PatchRecord& setUnitDimension(std::map< UnitDimension, double > const&);

private:
    PatchRecord();

    void flush_impl(std::string const&) override;
    void read() override;
};  //PatchRecord
} // openPMD
