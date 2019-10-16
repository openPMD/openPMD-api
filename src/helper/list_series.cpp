/* Copyright 2019-2020 Axel Huebl
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

#include "openPMD/helper/list_series.hpp"

#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"

#include <utility>
#include <iterator>
#include <set>
#include <string>


namespace openPMD
{
namespace helper
{
    std::ostream &
    listSeries(
        Series const & series,
        bool const longer,
        std::ostream & out
    )
    {
        out << "openPMD series: " << series.name() << "\n";
        out << "openPMD standard: " << series.openPMD() << "\n";
        out << "openPMD extensions: " << series.openPMDextension() << "\n\n";  //! @todo improve listing of extensions

        if( longer )
        {
            out << "data author: ";
            try{ out << series.author() << "\n"; } catch( no_such_attribute_error const & e ) { out << "unknown\n"; }
            out << "data created: ";
            try{ out << series.date() << "\n"; } catch( no_such_attribute_error const & e ) { out << "unknown\n"; }
            out << "data backend: " << series.backend() << "\n";
            out << "generating machine: ";
            try{ out << series.machine() << "\n"; } catch( no_such_attribute_error const & e ) { out << "unknown\n"; }
            out << "generating software: ";
            try{ out << series.software(); } catch( no_such_attribute_error const & e ) { out << "unknown"; }
            out <<  " (version: ";
            try{ out << series.softwareVersion() << ")\n"; } catch( no_such_attribute_error const & e ) { out << "unknown)\n"; }
            out << "generating software dependencies: ";
            try{ out << series.softwareDependencies() << "\n"; } catch( no_such_attribute_error const & e ) { out << "unknown\n"; }

            out << "\n";
        }

        std::set< std::string > meshes;     //! unique mesh names in all iterations
        std::set< std::string > particles;  //! unique particle species names in all iterations

        out << "number of iterations: " << series.iterations.size();
        if( longer )
            out << " (" << series.iterationEncoding() << ")";
        out << "\n";
        if( series.iterations.size() > 0u  )
        {
            if( longer )
                out << "  all iterations: ";

            for( auto const& i : series.iterations ) {
                if( longer )
                    out << i.first << " ";

                // find unique record names
                std::transform(
                    i.second.meshes.begin(),
                    i.second.meshes.end(),
                    std::inserter( meshes, meshes.end() ),
                    []( std::pair< std::string, Mesh > const & p )
                        { return p.first; }
                );
                std::transform(
                    i.second.particles.begin(),
                    i.second.particles.end(),
                    std::inserter( particles, particles.end() ),
                    []( std::pair< std::string, ParticleSpecies > const & p )
                        { return p.first; }
                );
            }

            if( longer )
                out << "\n";
        }

        out << "\n";
        out << "number of meshes: " << meshes.size() << "\n";
        if( longer && meshes.size() > 0u )
        {
            out << "  all meshes:\n";
            for( auto const& m : meshes )
                out << "    " << m << "\n";
        }

        out << "\n";
        out << "number of particle species: " << particles.size() << "\n";
        if( longer && particles.size() > 0u )
        {
            out << "  all particle species:\n";
            for( auto const& p : particles )
                out << "    " << p << "\n";
        }

        return out;
    }
} // helper
} // openPMD
