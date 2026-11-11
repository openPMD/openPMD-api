/* Copyright 2026 The openPMD Community
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

/** Enumerate all explicitly instantiated Container element types.
 *
 * The macro is invoked as MACRO(type, key_type). All container types use
 * std::string as key except Iteration, which is indexed by IterationIndex_t.
 */
#define OPENPMD_FOREACH_CONTAINER_TYPE(MACRO)                                  \
    MACRO(Mesh, std::string)                                                   \
    MACRO(MeshRecordComponent, std::string)                                    \
    MACRO(ParticlePatches, std::string)                                        \
    MACRO(ParticleSpecies, std::string)                                        \
    MACRO(PatchRecord, std::string)                                            \
    MACRO(PatchRecordComponent, std::string)                                   \
    MACRO(Record, std::string)                                                 \
    MACRO(RecordComponent, std::string)                                        \
    MACRO(CustomHierarchy, std::string)                                        \
    MACRO(CustomDataset, std::string)                                          \
    MACRO(Iteration, Iteration::IterationIndex_t)
