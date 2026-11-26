/* Copyright 2018-2025 Axel Huebl, Franz Poeschel
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
#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/Datatype.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include "openPMD/binding/python/Numpy.hpp"

#include <sstream>

void init_BaseRecordComponent(nb::module_ &m)
{
    using python::Self;
    using python::self_method;

    python::bind_class<BaseRecordComponent, Attributable>(
        m, "Base_Record_Component")
        .def(
            "__repr__",
            [](Self<BaseRecordComponent> brc) {
                std::stringstream ss;
                ss << "<openPMD.Base_Record_Component of '";
                ss << brc->getDatatype() << "'>";
                return ss.str();
            })

        .def("reset_datatype", self_method(&BaseRecordComponent::resetDatatype))
        .def(
            "available_chunks",
            self_method(&BaseRecordComponent::availableChunks))

        .def_prop_ro("unit_SI", self_method(&BaseRecordComponent::unitSI))
        .def_prop_ro("constant", self_method(&BaseRecordComponent::constant))
        .def_prop_ro("dtype", [](Self<BaseRecordComponent> brc) {
            return dtype_to_numpy(brc->getDatatype());
        });
}
