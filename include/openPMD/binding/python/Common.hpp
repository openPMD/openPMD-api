/* Copyright 2023-2025 The openPMD Community, Franz Poeschel
 *
 * This header is used to centrally define classes that shall not violate the
 * C++ one-definition-rule (ODR) for various Python translation units.
 *
 * Authors: Axel Huebl
 * License: LGPL-3.0-or-later
 */
#pragma once

#include "openPMD/ChunkInfo.hpp"
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
#include <pybind11/pytypes.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>

#if openPMD_USE_FILESYSTEM_HEADER
#include <pybind11/stl/filesystem.h>
#endif
// not yet used:
//   pybind11/functional.h  // for std::function

using PyVecChunkInfo = std::vector<openPMD::ChunkInfo>;

PYBIND11_MAKE_OPAQUE(openPMD::ChunkInfo)
PYBIND11_MAKE_OPAQUE(PyVecChunkInfo)
PYBIND11_MAKE_OPAQUE(openPMD::WrittenChunkInfo)
PYBIND11_MAKE_OPAQUE(openPMD::ChunkTable)
PYBIND11_MAKE_OPAQUE(openPMD::chunk_assignment::Assignment)
PYBIND11_MAKE_OPAQUE(openPMD::chunk_assignment::PartialAssignment)

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

namespace openPMD
{
template <typename Map, typename... Args>
auto bind_python_map(Args &&...args)
{
    using KeyType = typename Map::key_type;
    using MappedType = typename Map::mapped_type;
    auto res = py::bind_map<Map>(std::forward<Args>(args)...);
    if (!py::hasattr(res, "get"))
    {
        res.def(
               "get",
               [](Map &m,
                  KeyType const &key,
                  MappedType &default_val) -> MappedType & {
                   if (auto it = m.find(key); it != m.end())
                   {
                       return it->second;
                   }
                   else
                   {
                       return default_val;
                   }
               },
               py::return_value_policy::reference_internal // ref + keepalive
               )
            // .get() without default argument just added for completeness.
            // implementation is copied from pybind's __getitem__
            .def(
                "get",
                [](Map &m, const KeyType &k) -> MappedType & {
                    auto it = m.find(k);
                    if (it == m.end())
                    {
                        set_error(
                            PyExc_KeyError,
                            py::detail::format_message_key_error(k));
                        throw py::error_already_set();
                    }
                    return it->second;
                },
                py::return_value_policy::reference_internal // ref + keepalive
            );
    }
    return res;
}
} // namespace openPMD
