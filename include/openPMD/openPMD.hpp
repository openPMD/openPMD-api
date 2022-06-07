/* Copyright 2017-2021 Fabian Koller, Axel Huebl
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

/** Public definitions of openPMD-api
 */
namespace openPMD
{}

// IWYU pragma: begin_exports
#include "openPMD/Dataset.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/IterationEncoding.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/ReadIterations.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/UnitDimension.hpp"
#include "openPMD/WriteIterations.hpp"

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/backend/Writable.hpp"

#include "openPMD/IO/Access.hpp"
#include "openPMD/IO/Format.hpp"

#include "openPMD/auxiliary/Date.hpp"
#include "openPMD/auxiliary/DerefDynamicCast.hpp"
#include "openPMD/auxiliary/Option.hpp"
#include "openPMD/auxiliary/OutOfRangeMsg.hpp"
#include "openPMD/auxiliary/ShareRaw.hpp"
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/auxiliary/Variant.hpp"

#include "openPMD/helper/list_series.hpp"

#include "openPMD/config.hpp"
#include "openPMD/version.hpp"
// IWYU pragma: end_exports
