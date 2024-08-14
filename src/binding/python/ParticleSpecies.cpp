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
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Record.hpp"
#include "openPMD/Series.hpp"
#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"

#include "openPMD/binding/python/Common.hpp"
#include "openPMD/binding/python/Container.H"
#include "openPMD/binding/python/Pickle.hpp"

#include <sstream>
#include <string>
#include <vector>

void init_ParticleSpecies(py::module &m)
{
    auto py_ps_cnt = declare_container<PyPartContainer, Attributable>(
        m, "Particle_Container");

    py::class_<ParticleSpecies, Container<Record> > cl(m, "ParticleSpecies");
    cl.def(
          "__repr__",
          [](ParticleSpecies const &p) {
              std::stringstream stream;
              stream << "<openPMD.ParticleSpecies with " << p.size()
                     << " record(s) and " << p.numAttributes()
                     << " attribute(s)>";
              return stream.str();
          })

        .def_readwrite(
            "particle_patches",
            &ParticleSpecies::particlePatches,
            py::return_value_policy::copy,
            // garbage collection: return value must be freed before Series
            py::keep_alive<1, 0>());
    add_pickle(
        cl, [](openPMD::Series series, std::vector<std::string> const &group) {
            uint64_t const n_it = std::stoull(group.at(1));
            ParticleSpecies res =
                series.iterations[n_it].open().particles[group.at(3)];
            return internal::makeOwning(res, std::move(series));
        });

    finalize_container<PyPartContainer>(py_ps_cnt);
}
