/* Copyright 2018 Axel Huebl
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
#include <pybind11/stl_bind.h>
#include <pybind11/stl.h>

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Attribute.hpp"
#include "openPMD/auxiliary/Variant.hpp"

#include <string>
#include <vector>
#include <array>


// std::variant
//   https://pybind11.readthedocs.io/en/stable/advanced/cast/stl.html
namespace pybind11 {
namespace detail {
    template< typename... Ts >
    struct type_caster< variantSrc::variant< Ts... > > :
        variant_caster< variantSrc::variant< Ts... > >
    {};

} // namespace detail
} // namespace pybind11

namespace py = pybind11;
using namespace openPMD;

using PyAttributeKeys = std::vector< std::string >;
//PYBIND11_MAKE_OPAQUE(PyAttributeKeys)

void init_Attributable(py::module &m) {
    py::class_<Attributable>(m, "Attributable")
        .def(py::init<>())
        .def(py::init<Attributable const &>())

        .def("__repr__",
            [](Attributable const & attr) {
                return "<openPMD.Attributable with'" + std::to_string(attr.numAttributes()) + "' attributes>";
            }
        )

        .def_property_readonly(
            "attributes",
            []( Attributable & attr )
            {
                return attr.attributes();
            },
            // ref + keepalive
            py::return_value_policy::reference_internal
        )

        // C++ pass-through API
        .def("set_attribute", &Attributable::setAttribute< char >)
        .def("set_attribute", &Attributable::setAttribute< unsigned char >)
        .def("set_attribute", &Attributable::setAttribute< int16_t >)
        .def("set_attribute", &Attributable::setAttribute< int32_t >)
        .def("set_attribute", &Attributable::setAttribute< int64_t >)
        .def("set_attribute", &Attributable::setAttribute< uint16_t >)
        .def("set_attribute", &Attributable::setAttribute< uint32_t >)
        .def("set_attribute", &Attributable::setAttribute< uint64_t >)
        .def("set_attribute", &Attributable::setAttribute< float >)
        .def("set_attribute", &Attributable::setAttribute< double >)
        .def("set_attribute", &Attributable::setAttribute< long double >)
        .def("set_attribute", &Attributable::setAttribute< std::string >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< char > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< unsigned char > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< int16_t > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< int32_t > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< int64_t > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< uint16_t > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< uint32_t > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< uint64_t > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< float > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< double > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< long double > >)
        .def("set_attribute", &Attributable::setAttribute< std::vector< std::string > >)
        .def("set_attribute", &Attributable::setAttribute< std::array< double, 7 > >)
        .def("set_attribute", &Attributable::setAttribute< bool >)

        .def("get_attribute", []( Attributable & attr, std::string const& key ) {
            auto v = attr.getAttribute(key);
            return v.getResource();
        })
        .def("delete_attribute", &Attributable::deleteAttribute)
        .def("contains_attribute", &Attributable::containsAttribute)

        .def("__len__", &Attributable::numAttributes)

        // @todo _ipython_key_completions_ if we find a way to add a [] interface

        .def_property_readonly("comment", &Attributable::comment)
        .def("set_comment", &Attributable::setComment)
    ;

    py::bind_vector< PyAttributeKeys >(
        m,
        "Attribute_Keys"
    );
}
