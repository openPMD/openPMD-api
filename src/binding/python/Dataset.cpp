/* Copyright 2018-2025 Axel Huebl, Franz Poeschel, Junmin Gu
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
#include "openPMD/Dataset.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Numpy.hpp"
#include "openPMD/binding/python/auxiliary.hpp"

#include <string>

namespace internal
{
struct DefineDatasetConstructor
{
    static constexpr auto resolve_datatype(Datatype dt) -> Datatype
    {
        return dt;
    }

    static auto resolve_datatype(py::dtype dt) -> Datatype
    {
        return dtype_from_numpy(std::move(dt));
    }

    static auto resolve_datatype(py::object const &dt) -> Datatype
    {
        return dtype_from_numpy(dt);
    }

    static constexpr auto resolve_options(std::string const &str)
        -> std::string const &
    {
        return str;
    }

    static auto resolve_options(py::object const &obj) -> std::string
    {
        return ::auxiliary::json_dumps(obj);
    }

    template <typename Datatype_t, typename Options_t>
    static auto call(py::class_<Dataset> &ds) -> py::class_<Dataset> &
    {
        auto options_arg = []() {
            if constexpr (std::is_same_v<Options_t, std::string>)
            {
                return (py::arg("options") = "{}");
            }
            else
            {
                return py::arg("options");
            }
        }();
        return ds.def(
            py::init([](Datatype_t dt, Extent e, Options_t const &options) {
                auto resolved_dtype =
                    resolve_datatype(std::forward<Datatype_t>(dt));
                decltype(auto) resolved_options = resolve_options(options);
                return new Dataset{
                    resolved_dtype, std::move(e), resolved_options};
            }),
            py::arg("dtype"),
            py::arg("extent"),
            options_arg);
    }
};
} // namespace internal

void init_Dataset(py::module &m)
{
    auto pyDataset =
        py::class_<Dataset>(m, "Dataset")
            .def(py::init<Extent>(), py::arg("extent"))
            .def(
                "__repr__",
                [](const Dataset &d) {
                    std::stringstream stream;
                    stream << "<openPMD.Dataset of type '" << d.dtype
                           << "' and with extent ";
                    if (d.extent.empty())
                    {
                        stream << "[]>";
                    }
                    else
                    {
                        auto begin = d.extent.begin();
                        stream << '[' << *begin++;
                        for (; begin != d.extent.end(); ++begin)
                        {
                            stream << ", " << *begin;
                        }
                        stream << "]>";
                    }
                    return stream.str();
                })

            .def_property_readonly(
                "joined_dimension",
                py::overload_cast<>(&Dataset::joinedDimension, py::const_))
            .def_readonly("extent", &Dataset::extent)
            .def("extend", &Dataset::extend)
            .def_readonly("rank", &Dataset::rank)
            .def_property_readonly(
                "dtype",
                [](const Dataset &d) { return dtype_to_numpy(d.dtype); })
            .def_readwrite("options", &Dataset::options);
    ::auxiliary::ForEachTypeNested<
        ::internal::DefineDatasetConstructor,
        // types for Datatype param
        std::tuple<Datatype, py::dtype, py::object const &>,
        // types for options param
        std::tuple<std::string, py::object>>::call(pyDataset);
    pyDataset.attr("JOINED_DIMENSION") =
        py::int_(uint64_t(Dataset::JOINED_DIMENSION));
}
