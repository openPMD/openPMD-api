/* Copyright 2018-2021 Fabian Koller
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
#include "openPMD/IO/IOTask.hpp"
#include "openPMD/auxiliary/JSON_internal.hpp"
#include "openPMD/backend/Attributable.hpp"

#include <iostream> // std::cerr


namespace openPMD
{
Writable*
getWritable(Attributable* a)
{ return &a->writable(); }

template<>
void Parameter< Operation::CREATE_DATASET >::warnUnusedParameters<
    json::TracingJSON >(
    json::TracingJSON & config,
    std::string const & currentBackendName,
    std::string const & warningMessage )
{
    /*
     * Fake-read non-backend-specific options. Some backends don't read those
     * and we don't want to have warnings for them.
     */
    for( std::string const & key : { "resizable" } )
    {
        config[ key ];
    }

    auto shadow = config.invertShadow();
    // The backends are supposed to deal with this
    // Only global options here
    for( auto const & backendKey : json::backendKeys )
    {
        if( backendKey != currentBackendName )
        {
            shadow.erase( backendKey );
        }
    }
    if( shadow.size() > 0 )
    {
        switch( config.originallySpecifiedAs )
        {
        case json::SupportedLanguages::JSON:
            std::cerr << warningMessage << shadow.dump() << std::endl;
            break;
        case json::SupportedLanguages::TOML:
        {
            auto asToml = json::jsonToToml( shadow );
            std::cerr << warningMessage << asToml << std::endl;
            break;
        }
        }
    }
}
} // openPMD
