/* Copyright 2017-2019 Fabian Koller
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
#pragma once

#include <string>
#include <memory>

// expose private and protected members for invasive testing
#ifndef OPENPMD_private
#   define OPENPMD_private private
#endif


namespace openPMD
{
namespace test
{
struct TestHelper;
} // namespace test
class AbstractFilePosition;
class AbstractIOHandler;
class Attributable;
struct ADIOS2FilePosition;
template <typename FilePositionType>
class AbstractIOHandlerImplCommon;


/** @brief Layer to mirror structure of logical data and persistent data in file.
 *
 * Hierarchy of objects (datasets, groups, attributes, ...) in openPMD is
 * managed in this class.
 * It also indicates the current synchronization state between logical
 * and persistent data: - whether the object has been created in persistent form
 *                      - whether the logical object has been modified compared
 *                        to last persistent state
 */
class Writable
{
    friend class Attributable;
    template< typename T_elem >
    friend class BaseRecord;
    template<
            typename T,
            typename T_key,
            typename T_container
    >
    friend class Container;
    friend class Iteration;
    friend class Mesh;
    friend class ParticleSpecies;
    friend class Series;
    friend class Record;
    friend class ADIOS1IOHandlerImpl;
    friend class ParallelADIOS1IOHandlerImpl;
    friend class ADIOS2IOHandlerImpl;
    friend class HDF5IOHandlerImpl;
    friend class ParallelHDF5IOHandlerImpl;
    friend class AbstractIOHandlerImplCommon<ADIOS2FilePosition>;
    friend class JSONIOHandlerImpl;
    friend struct test::TestHelper;
    friend std::string concrete_h5_file_position(Writable*);
    friend std::string concrete_bp1_file_position(Writable*);

public:
    Writable(Attributable* = nullptr);
    virtual ~Writable();

OPENPMD_private:
    std::shared_ptr< AbstractFilePosition > abstractFilePosition;
    std::shared_ptr< AbstractIOHandler > IOHandler;
    Attributable* attributable;
    Writable* parent;
    bool dirty;
    bool written;
};
} // namespace openPMD
