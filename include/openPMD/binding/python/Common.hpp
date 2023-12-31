/* Copyright 2023 The openPMD Community
 *
 * This header is used to centrally define classes that shall not violate the
 * C++ one-definition-rule (ODR) for various Python translation units.
 *
 * Authors: Axel Huebl
 * License: LGPL-3.0-or-later
 */
#pragma once

#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/RecordComponent.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/backend/BaseRecord.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"
#include "openPMD/backend/PatchRecord.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"

#include <pybind11/gil.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
// not yet used:
//   pybind11/functional.h  // for std::function

// used exclusively in all our Python .cpp files
namespace py = pybind11;
using namespace openPMD;

// opaque types
using PyIterationContainer = Series::IterationsContainer_t;
using PyMeshContainer = Container<Mesh>;
using PyPartContainer = Container<ParticleSpecies>;
using PyPatchContainer = Container<ParticlePatches>;
using PyRecordContainer = Container<Record>;
using PyPatchRecordContainer = Container<PatchRecord>;
using PyRecordComponentContainer = Container<RecordComponent>;
using PyMeshRecordComponentContainer = Container<MeshRecordComponent>;
using PyPatchRecordComponentContainer = Container<PatchRecordComponent>;
using PyBaseRecordRecordComponent = BaseRecord<RecordComponent>;
using PyBaseRecordMeshRecordComponent = BaseRecord<MeshRecordComponent>;
using PyBaseRecordPatchRecordComponent = BaseRecord<PatchRecordComponent>;
PYBIND11_MAKE_OPAQUE(PyIterationContainer)
PYBIND11_MAKE_OPAQUE(PyMeshContainer)
PYBIND11_MAKE_OPAQUE(PyPartContainer)
PYBIND11_MAKE_OPAQUE(PyPatchContainer)
PYBIND11_MAKE_OPAQUE(PyRecordContainer)
PYBIND11_MAKE_OPAQUE(PyPatchRecordContainer)
PYBIND11_MAKE_OPAQUE(PyRecordComponentContainer)
PYBIND11_MAKE_OPAQUE(PyMeshRecordComponentContainer)
PYBIND11_MAKE_OPAQUE(PyPatchRecordComponentContainer)
PYBIND11_MAKE_OPAQUE(PyBaseRecordRecordComponent)
PYBIND11_MAKE_OPAQUE(PyBaseRecordPatchRecordComponent)
