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
#include <memory>
#include <iostream>

#include "Auxiliary.hpp"
#include "Dataset.hpp"
#include "Iteration.hpp"
#include "Series.hpp"


Iteration::Iteration()
        : meshes{Container< Mesh >()},
          particles{Container< ParticleSpecies >()}
{
    setTime(static_cast< double >(0));
    setDt(static_cast< double >(1));
    setTimeUnitSI(1);
}

Iteration::Iteration(Iteration const& i)
        : Attributable{i},
          meshes{i.meshes},
          particles{i.particles}
{
    IOHandler = i.IOHandler;
    parent = i.parent;
    meshes.IOHandler = IOHandler;
    meshes.parent = this;
    particles.IOHandler = IOHandler;
    particles.parent = this;
}

template< typename T >
Iteration&
Iteration::setTime(T time)
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    setAttribute("time", time);
    dirty = true;
    return *this;
}

template< typename T >
Iteration&
Iteration::setDt(T dt)
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    setAttribute("dt", dt);
    dirty = true;
    return *this;
}

double
Iteration::timeUnitSI() const
{
    return getAttribute("timeUnitSI").get< double >();
}

Iteration&
Iteration::setTimeUnitSI(double timeUnitSI)
{
    setAttribute("timeUnitSI", timeUnitSI);
    dirty = true;
    return *this;
}

void
Iteration::flushFileBased(uint64_t i)
{
    if( !written )
    {
        /* create file */
        Series* o = dynamic_cast<Series *>(parent->parent);
        Parameter< Operation::CREATE_FILE > file_parameter;
        file_parameter.name = replace_first(o->iterationFormat(), "%T", std::to_string(i));
        IOHandler->enqueue(IOTask(o, file_parameter));
        IOHandler->flush();

        /* create basePath */
        Parameter< Operation::CREATE_PATH > path_parameter;
        path_parameter.path = replace_first(o->basePath(), "%T/", "");
        IOHandler->enqueue(IOTask(&o->iterations, path_parameter));
        IOHandler->flush();

        /* create iteration path */
        path_parameter.path = std::to_string(i);
        IOHandler->enqueue(IOTask(this, path_parameter));
        IOHandler->flush();
    } else
    {
        /* open file */
        Series* o = dynamic_cast<Series *>(parent->parent);
        Parameter< Operation::OPEN_FILE > file_parameter;
        file_parameter.name = replace_last(o->iterationFormat(), "%T", std::to_string(i));
        IOHandler->enqueue(IOTask(o, file_parameter));
        IOHandler->flush();

        /* open basePath */
        Parameter< Operation::OPEN_PATH > path_parameter;
        path_parameter.path = replace_first(o->basePath(), "%T/", "");
        IOHandler->enqueue(IOTask(&o->iterations, path_parameter));
        IOHandler->flush();

        /* open iteration path */
        path_parameter.path = std::to_string(i);
        IOHandler->enqueue(IOTask(this, path_parameter));
        IOHandler->flush();
    }

    flush();
}

void
Iteration::flushGroupBased(uint64_t i)
{
    if( !written )
    {
        /* create iteration path */
        Parameter< Operation::CREATE_PATH > iteration_parameter;
        iteration_parameter.path = std::to_string(i);
        IOHandler->enqueue(IOTask(this, iteration_parameter));
        IOHandler->flush();
    }

    flush();
}

void
Iteration::flush()
{
    /* Find the root point [Series] of this file,
     * meshesPath and particlesPath are stored there */
    Writable *w = this;
    while( w->parent )
        w = w->parent;
    Series* o = dynamic_cast<Series *>(w);

    meshes.flush(o->meshesPath());
    for( auto& m : meshes )
        m.second.flush(m.first);

    particles.flush(o->particlesPath());
    for( auto& species : particles )
    {
        species.second.flush(species.first);
        for( auto& record : species.second )
            record.second.flush(record.first);
    }

    flushAttributes();
}

void
Iteration::read()
{
    /* allow all attributes to be set */
    written = false;

    using DT = Datatype;
    Parameter< Operation::READ_ATT > attribute_parameter;

    attribute_parameter.name = "dt";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::FLOAT )
        setDt(Attribute(*attribute_parameter.resource).get< float >());
    else if( *attribute_parameter.dtype == DT::DOUBLE )
        setDt(Attribute(*attribute_parameter.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'dt'");

    attribute_parameter.name = "time";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::FLOAT )
        setTime(Attribute(*attribute_parameter.resource).get< float >());
    else if( *attribute_parameter.dtype == DT::DOUBLE )
        setTime(Attribute(*attribute_parameter.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'time'");

    attribute_parameter.name = "timeUnitSI";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::DOUBLE )
        setTimeUnitSI(Attribute(*attribute_parameter.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'timeUnitSI'");

    /* Find the root point [Series] of this file,
     * meshesPath and particlesPath are stored there */
    Writable *w = this;
    while( w->parent )
        w = w->parent;
    Series* o = dynamic_cast<Series *>(w);

    meshes.clear_unchecked();
    Parameter< Operation::OPEN_PATH > path_parameter;
    path_parameter.path = o->meshesPath();
    IOHandler->enqueue(IOTask(&meshes, path_parameter));
    IOHandler->flush();

    meshes.readAttributes();

    /* obtain all non-scalar meshes */
    Parameter< Operation::LIST_PATHS > plist_parameter;
    IOHandler->enqueue(IOTask(&meshes, plist_parameter));
    IOHandler->flush();

    Parameter< Operation::LIST_ATTS > alist_parameter;
    for( auto const& mesh_name : *plist_parameter.paths )
    {
        Mesh& m = meshes[mesh_name];
        path_parameter.path = mesh_name;
        alist_parameter.attributes->clear();
        IOHandler->enqueue(IOTask(&m, path_parameter));
        IOHandler->enqueue(IOTask(&m, alist_parameter));
        IOHandler->flush();

        auto begin = alist_parameter.attributes->begin();
        auto end = alist_parameter.attributes->end();
        auto value = std::find(begin, end, "value");
        auto shape = std::find(begin, end, "shape");
        if( value != end && shape != end )
        {
            MeshRecordComponent& mrc = m[MeshRecordComponent::SCALAR];
            mrc.m_isConstant = true;
            mrc.parent = m.parent;
            mrc.abstractFilePosition = m.abstractFilePosition;
        }
        m.read();
    }

    /* obtain all scalar meshes */
    Parameter< Operation::LIST_DATASETS > dlist_parameter;
    IOHandler->enqueue(IOTask(&meshes, dlist_parameter));
    IOHandler->flush();

    Parameter< Operation::OPEN_DATASET > dataset_parameter;
    for( auto const& mesh_name : *dlist_parameter.datasets )
    {
        Mesh& m = meshes[mesh_name];
        dataset_parameter.name = mesh_name;
        IOHandler->enqueue(IOTask(&m, dataset_parameter));
        IOHandler->flush();
        MeshRecordComponent& mrc = m[MeshRecordComponent::SCALAR];
        mrc.abstractFilePosition = m.abstractFilePosition;
        mrc.parent = m.parent;
        mrc.written = true;
        mrc.resetDataset(Dataset(*dataset_parameter.dtype, *dataset_parameter.extent));
        m.read();
    }

    particles.clear_unchecked();
    path_parameter.path = o->particlesPath();
    IOHandler->enqueue(IOTask(&particles, path_parameter));
    IOHandler->flush();

    particles.readAttributes();

    /* obtain all particle species */
    plist_parameter.paths->clear();
    IOHandler->enqueue(IOTask(&particles, plist_parameter));
    IOHandler->flush();

    for( auto const& species_name : *plist_parameter.paths )
    {
        ParticleSpecies& p = particles[species_name];
        path_parameter.path = species_name;
        IOHandler->enqueue(IOTask(&p, path_parameter));
        IOHandler->flush();
        p.read();
    }

    readAttributes();

    /* this file need not be flushed */
    meshes.written = true;
    particles.written = true;
    written = true;
}


