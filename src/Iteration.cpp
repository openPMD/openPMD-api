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
#include "auxiliary/StringManip.hpp"
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
        Parameter< Operation::CREATE_FILE > fCreate;
        fCreate.name = replace_first(o->iterationFormat(), "%T", std::to_string(i));
        IOHandler->enqueue(IOTask(o, fCreate));
        IOHandler->flush();

        /* create basePath */
        Parameter< Operation::CREATE_PATH > pCreate;
        pCreate.path = replace_first(o->basePath(), "%T/", "");
        IOHandler->enqueue(IOTask(&o->iterations, pCreate));
        IOHandler->flush();

        /* create iteration path */
        pCreate.path = std::to_string(i);
        IOHandler->enqueue(IOTask(this, pCreate));
        IOHandler->flush();
    } else
    {
        /* open file */
        Series* o = dynamic_cast<Series *>(parent->parent);
        Parameter< Operation::OPEN_FILE > fOpen;
        fOpen.name = replace_last(o->iterationFormat(), "%T", std::to_string(i));
        IOHandler->enqueue(IOTask(o, fOpen));
        IOHandler->flush();

        /* open basePath */
        Parameter< Operation::OPEN_PATH > pOpen;
        pOpen.path = replace_first(o->basePath(), "%T/", "");
        IOHandler->enqueue(IOTask(&o->iterations, pOpen));
        IOHandler->flush();

        /* open iteration path */
        pOpen.path = std::to_string(i);
        IOHandler->enqueue(IOTask(this, pOpen));
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
        Parameter< Operation::CREATE_PATH > pCreate;
        pCreate.path = std::to_string(i);
        IOHandler->enqueue(IOTask(this, pCreate));
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
    Parameter< Operation::READ_ATT > aRead;

    aRead.name = "dt";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    if( *aRead.dtype == DT::FLOAT )
        setDt(Attribute(*aRead.resource).get< float >());
    else if( *aRead.dtype == DT::DOUBLE )
        setDt(Attribute(*aRead.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'dt'");

    aRead.name = "time";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    if( *aRead.dtype == DT::FLOAT )
        setTime(Attribute(*aRead.resource).get< float >());
    else if( *aRead.dtype == DT::DOUBLE )
        setTime(Attribute(*aRead.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'time'");

    aRead.name = "timeUnitSI";
    IOHandler->enqueue(IOTask(this, aRead));
    IOHandler->flush();
    if( *aRead.dtype == DT::DOUBLE )
        setTimeUnitSI(Attribute(*aRead.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'timeUnitSI'");

    /* Find the root point [Series] of this file,
     * meshesPath and particlesPath are stored there */
    Writable *w = this;
    while( w->parent )
        w = w->parent;
    Series* o = dynamic_cast<Series *>(w);

    meshes.clear_unchecked();
    Parameter< Operation::OPEN_PATH > pOpen;
    pOpen.path = o->meshesPath();
    IOHandler->enqueue(IOTask(&meshes, pOpen));
    IOHandler->flush();

    meshes.readAttributes();

    /* obtain all non-scalar meshes */
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler->enqueue(IOTask(&meshes, pList));
    IOHandler->flush();

    Parameter< Operation::LIST_ATTS > aList;
    for( auto const& mesh_name : *pList.paths )
    {
        Mesh& m = meshes[mesh_name];
        pOpen.path = mesh_name;
        aList.attributes->clear();
        IOHandler->enqueue(IOTask(&m, pOpen));
        IOHandler->enqueue(IOTask(&m, aList));
        IOHandler->flush();

        auto begin = aList.attributes->begin();
        auto end = aList.attributes->end();
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
    Parameter< Operation::LIST_DATASETS > dList;
    IOHandler->enqueue(IOTask(&meshes, dList));
    IOHandler->flush();

    Parameter< Operation::OPEN_DATASET > dOpen;
    for( auto const& mesh_name : *dList.datasets )
    {
        Mesh& m = meshes[mesh_name];
        dOpen.name = mesh_name;
        IOHandler->enqueue(IOTask(&m, dOpen));
        IOHandler->flush();
        MeshRecordComponent& mrc = m[MeshRecordComponent::SCALAR];
        mrc.abstractFilePosition = m.abstractFilePosition;
        mrc.parent = m.parent;
        mrc.written = false;
        mrc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        mrc.written = true;
        m.read();
    }

    particles.clear_unchecked();
    pOpen.path = o->particlesPath();
    IOHandler->enqueue(IOTask(&particles, pOpen));
    IOHandler->flush();

    particles.readAttributes();

    /* obtain all particle species */
    pList.paths->clear();
    IOHandler->enqueue(IOTask(&particles, pList));
    IOHandler->flush();

    for( auto const& species_name : *pList.paths )
    {
        ParticleSpecies& p = particles[species_name];
        pOpen.path = species_name;
        IOHandler->enqueue(IOTask(&p, pOpen));
        IOHandler->flush();
        p.read();
    }

    readAttributes();

    /* this file need not be flushed */
    meshes.written = true;
    particles.written = true;
    written = true;
}


template
float Iteration::time< float >() const;
template
double Iteration::time< double >() const;
template
long double Iteration::time< long double >() const;

template
float Iteration::dt< float >() const;
template
double Iteration::dt< double >() const;
template
long double Iteration::dt< long double >() const;
