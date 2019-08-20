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
#include <openPMD/backend/Writable.hpp>
#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/Dataset.hpp"
#include "openPMD/Datatype.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Series.hpp"


namespace openPMD
{
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
    meshes.parent = this->m_writable.get();
    particles.IOHandler = IOHandler;
    particles.parent = this->m_writable.get();
}

template< typename T >
Iteration&
Iteration::setTime(T newTime)
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    setAttribute("time", newTime);
    return *this;
}

template< typename T >
Iteration&
Iteration::setDt(T newDt)
{
    static_assert(std::is_floating_point< T >::value, "Type of attribute must be floating point");

    setAttribute("dt", newDt);
    return *this;
}

double
Iteration::timeUnitSI() const
{
    return getAttribute("timeUnitSI").get< double >();
}

Iteration&
Iteration::setTimeUnitSI(double newTimeUnitSI)
{
    setAttribute("timeUnitSI", newTimeUnitSI);
    return *this;
}

void
Iteration::flushFileBased(std::string const& filename, uint64_t i)
{
    if( !written )
    {
        /* create file */
        auto s = dynamic_cast< Series* >(parent->attributable->parent->attributable);
        Parameter< Operation::CREATE_FILE > fCreate;
        fCreate.name = filename;
        IOHandler->enqueue(IOTask(s, fCreate));

        /* create basePath */
        Parameter< Operation::CREATE_PATH > pCreate;
        pCreate.path = auxiliary::replace_first(s->basePath(), "%T/", "");
        IOHandler->enqueue(IOTask(&s->iterations, pCreate));

        /* create iteration path */
        pCreate.path = std::to_string(i);
        IOHandler->enqueue(IOTask(this, pCreate));
    } else
    {
        /* open file */
        auto s = dynamic_cast< Series* >(parent->attributable->parent->attributable);
        Parameter< Operation::OPEN_FILE > fOpen;
        fOpen.name = filename;
        IOHandler->enqueue(IOTask(s, fOpen));

        /* open basePath */
        Parameter< Operation::OPEN_PATH > pOpen;
        pOpen.path = auxiliary::replace_first(s->basePath(), "%T/", "");
        IOHandler->enqueue(IOTask(&s->iterations, pOpen));

        /* open iteration path */
        pOpen.path = std::to_string(i);
        IOHandler->enqueue(IOTask(this, pOpen));
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
    }

    flush();
}

void
Iteration::flush()
{
    if( IOHandler->accessTypeFrontend == AccessType::READ_ONLY )
    {
        for( auto& m : meshes )
            m.second.flush(m.first);
        for( auto& species : particles )
            species.second.flush(species.first);
    } else
    {
        /* Find the root point [Series] of this file,
         * meshesPath and particlesPath are stored there */
        Writable *w = this->parent;
        while( w->parent )
            w = w->parent;
        auto s = dynamic_cast< Series* >(w->attributable);

        if( !meshes.empty() || s->containsAttribute("meshesPath") )
        {
            if( !s->containsAttribute("meshesPath") )
                s->setMeshesPath("meshes/");
            s->flushMeshesPath();
            meshes.flush(s->meshesPath());
            for( auto& m : meshes )
                m.second.flush(m.first);
        }

        if( !particles.empty() || s->containsAttribute("particlesPath") )
        {
            if( !s->containsAttribute("particlesPath") )
                s->setParticlesPath("particles/");
            s->flushParticlesPath();
            particles.flush(s->particlesPath());
            for( auto& species : particles )
                species.second.flush(species.first);
        }

        flushAttributes();
    }
}

void
Iteration::read()
{
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
    Writable *w = getWritable(this);
    while( w->parent )
        w = w->parent;
    auto s = dynamic_cast< Series* >(w->attributable);

    Parameter< Operation::LIST_PATHS > pList;
    std::string version = s->openPMD();
    bool hasMeshes = false;
    bool hasParticles = false;
    if( version == "1.0.0" || version == "1.0.1" )
    {
        IOHandler->enqueue(IOTask(this, pList));
        IOHandler->flush();
        hasMeshes = std::count(
            pList.paths->begin(),
            pList.paths->end(),
            auxiliary::replace_last(s->meshesPath(), "/", "")
        ) == 1;
        hasParticles = std::count(
            pList.paths->begin(),
            pList.paths->end(),
            auxiliary::replace_last(s->particlesPath(), "/", "")
        ) == 1;
        pList.paths->clear();
    } else
    {
        hasMeshes = s->containsAttribute("meshesPath");
        hasParticles = s->containsAttribute("particlesPath");
    }

    if( hasMeshes )
    {
        Parameter< Operation::OPEN_PATH > pOpen;
        pOpen.path = s->meshesPath();
        IOHandler->enqueue(IOTask(&meshes, pOpen));

        meshes.readAttributes();

        /* obtain all non-scalar meshes */
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

            auto att_begin = aList.attributes->begin();
            auto att_end = aList.attributes->end();
            auto value = std::find(att_begin, att_end, "value");
            auto shape = std::find(att_begin, att_end, "shape");
            if( value != att_end && shape != att_end )
            {
                MeshRecordComponent& mrc = m[MeshRecordComponent::SCALAR];
                mrc.parent = m.parent;
                IOHandler->enqueue(IOTask(&mrc, pOpen));
                IOHandler->flush();
                *mrc.m_isConstant = true;
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
            mrc.parent = m.parent;
            IOHandler->enqueue(IOTask(&mrc, dOpen));
            IOHandler->flush();
            mrc.written = false;
            mrc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
            mrc.written = true;
            m.read();
        }
    }

    if( hasParticles )
    {
        Parameter< Operation::OPEN_PATH > pOpen;
        pOpen.path = s->particlesPath();
        IOHandler->enqueue(IOTask(&particles, pOpen));

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
    }

    readAttributes();
}

void
Iteration::linkHierarchy(std::shared_ptr< Writable > const& w)
{
    Attributable::linkHierarchy(w);
    meshes.linkHierarchy(m_writable);
    particles.linkHierarchy(m_writable);
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

template
Iteration& Iteration::setTime< float >(float time);
template
Iteration& Iteration::setTime< double >(double time);
template
Iteration& Iteration::setTime< long double >(long double time);

template
Iteration& Iteration::setDt< float >(float dt);
template
Iteration& Iteration::setDt< double >(double dt);
template
Iteration& Iteration::setDt< long double >(long double dt);
} // openPMD
