/* Copyright 2018-2021 Axel Huebl
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
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "openPMD/DatatypeHelpers.hpp"
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/PatchRecordComponent.hpp"
#include "openPMD/binding/python/Numpy.hpp"

namespace py = pybind11;
using namespace openPMD;

namespace
{
struct Prc_Load
{
    template <typename T>
    static void call(PatchRecordComponent &prc, py::array &a)
    {
        prc.loadRaw<T>((T *)a.mutable_data());
    }

    static constexpr char const *errorMsg = "Datatype not known in 'load'!";
};
} // namespace

void init_PatchRecordComponent(py::module &m)
{
    py::class_<PatchRecordComponent, BaseRecordComponent>(
        m, "Patch_Record_Component")
        .def_property(
            "unit_SI",
            &BaseRecordComponent::unitSI,
            &PatchRecordComponent::setUnitSI)

        .def("reset_dataset", &PatchRecordComponent::resetDataset)
        .def_property_readonly(
            "ndims", &PatchRecordComponent::getDimensionality)
        .def_property_readonly("shape", &PatchRecordComponent::getExtent)

        .def(
            "load",
            [](PatchRecordComponent &prc) {
                auto const dtype = dtype_to_numpy(prc.getDatatype());
                auto a = py::array(dtype, prc.getExtent()[0]);

                switchNonVectorType<Prc_Load>(prc.getDatatype(), prc, a);

                return a;
            })

        // all buffer types
        .def(
            "store",
            [](PatchRecordComponent &prc, uint64_t idx, py::buffer a) {
                py::buffer_info buf = a.request();
                auto const dtype = dtype_from_bufferformat(buf.format);

                using DT = Datatype;

                // allow one-element n-dimensional buffers as well
                py::ssize_t numElements = 1;
                if (buf.ndim > 0)
                {
                    for (auto d = 0; d < buf.ndim; ++d)
                        numElements *= buf.shape.at(d);
                }

                // Numpy: Handling of arrays and scalars
                // work-around for
                // https://github.com/pybind/pybind11/issues/1224
                // -> passing numpy scalars as buffers needs numpy 1.15+
                //    https://github.com/numpy/numpy/issues/10265
                //    https://github.com/pybind/pybind11/issues/1224#issuecomment-354357392
                // scalars, see PEP 3118
                // requires Numpy 1.15+
                if (numElements == 1)
                {
                    // refs:
                    //   https://docs.scipy.org/doc/numpy-1.15.0/reference/arrays.interface.html
                    //   https://docs.python.org/3/library/struct.html#format-characters
                    // std::cout << "  scalar type '" << buf.format << "'" <<
                    // std::endl; typestring: encoding + type + number of bytes
                    switch (dtype)
                    {
                    case DT::BOOL:
                        return prc.store(idx, *static_cast<bool *>(buf.ptr));
                        break;
                    case DT::SHORT:
                        return prc.store(idx, *static_cast<short *>(buf.ptr));
                        break;
                    case DT::INT:
                        return prc.store(idx, *static_cast<int *>(buf.ptr));
                        break;
                    case DT::LONG:
                        return prc.store(idx, *static_cast<long *>(buf.ptr));
                        break;
                    case DT::LONGLONG:
                        return prc.store(
                            idx, *static_cast<long long *>(buf.ptr));
                        break;
                    case DT::USHORT:
                        return prc.store(
                            idx, *static_cast<unsigned short *>(buf.ptr));
                        break;
                    case DT::UINT:
                        return prc.store(
                            idx, *static_cast<unsigned int *>(buf.ptr));
                        break;
                    case DT::ULONG:
                        return prc.store(
                            idx, *static_cast<unsigned long *>(buf.ptr));
                        break;
                    case DT::ULONGLONG:
                        return prc.store(
                            idx, *static_cast<unsigned long long *>(buf.ptr));
                        break;
                    case DT::FLOAT:
                        return prc.store(idx, *static_cast<float *>(buf.ptr));
                        break;
                    case DT::DOUBLE:
                        return prc.store(idx, *static_cast<double *>(buf.ptr));
                        break;
                    case DT::LONG_DOUBLE:
                        return prc.store(
                            idx, *static_cast<long double *>(buf.ptr));
                        break;
                    default:
                        throw std::runtime_error(
                            "store: "
                            "Unknown Datatype!");
                    }
                }
                else
                {
                    throw std::runtime_error(
                        "store: "
                        "Only scalar values supported!");
                }
            },
            py::arg("idx"),
            py::arg("data"))
        // allowed python intrinsics, after (!) buffer matching
        .def(
            "store",
            &PatchRecordComponent::store<double>,
            py::arg("idx"),
            py::arg("data"))
        .def(
            "store",
            &PatchRecordComponent::store<long>,
            py::arg("idx"),
            py::arg("data"))

        // TODO implement convenient, patch-object level store/load

        // TODO remove in future versions (deprecated)
        .def("set_unit_SI", &PatchRecordComponent::setUnitSI);
}
