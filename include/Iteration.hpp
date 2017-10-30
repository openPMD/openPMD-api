/* Copyright 2017 Fabian Koller
 *
 * This file is part of libopenPMD.
 *
 * libopenPMD is free software: you can redistribute it and/or modify
 * it under the terms of of either the GNU General Public License or
 * the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * libopenPMD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License and the GNU Lesser General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and the GNU Lesser General Public License along with libopenPMD.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once


#include <map>
#include <string>
#include <vector>

#include "Attributable.hpp"
#include "Container.hpp"
#include "Mesh.hpp"
#include "ParticleSpecies.hpp"


/** @brief  Logical compilation of data from one timeframe (e.g. a simulation step).
 */
class Iteration : public Attributable
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Series;

public:
    Iteration(Iteration const&);

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float, double).
     * @return  Global reference time for this iteration.
     */
    template< typename T >
    T time() const;
    /** Set the global reference time for this iteration.
     *
     * @tparam  T       Floating point type of user-selected precision (e.g. float, double).
     * @param   time    Global reference time for this iteration.
     * @return  Reference to modified iteration.
     */
    template< typename T >
    Iteration& setTime(T time);

    /**
     * @tparam  T   Floating point type of user-selected precision (e.g. float, double).
     * @return  Time step used to reach this iteration.
     */
    template< typename T >
    T dt() const;
    /** Set the time step used to reach this iteration.
     *
     * @tparam  T   Floating point type of user-selected precision (e.g. float, double).
     * @param   dt  Time step used to reach this iteration.
     * @return  Reference to modified iteration.
     */
    template< typename T >
    Iteration& setDt(T dt);

    /**
     * @return Conversion factor to convert time and dt to seconds
     */
    double timeUnitSI() const;
    Iteration& setTimeUnitSI(double);

    Container< Mesh > meshes;
    Container< ParticleSpecies > particles; //particleSpecies?

private:
    Iteration();
    void flushFileBased(uint64_t);
    void flushGroupBased(uint64_t);
    void flush();
    void read();
};  //Iteration

template< typename T >
inline T
Iteration::time() const
{ return readFloatingpoint< T >("time"); }

template< typename T >
inline T
Iteration::dt() const
{ return readFloatingpoint< T >("dt"); }
