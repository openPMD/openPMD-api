#include <memory>
#include <iostream>

#include "../include/Auxiliary.hpp"
#include "../include/Dataset.hpp"
#include "../include/Iteration.hpp"
#include "../include/Output.hpp"


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
        Output* o = dynamic_cast<Output *>(parent);
        Parameter< Operation::CREATE_FILE > file_parameter;
        file_parameter.name = replace_first(o->iterationFormat(), "%T", std::to_string(i));
        IOHandler->enqueue(IOTask(this, file_parameter));
        IOHandler->flush();
        o->abstractFilePosition = abstractFilePosition;
        o->written = true;

        /* create basePath */
        Parameter< Operation::CREATE_PATH > iteration_parameter;
        iteration_parameter.path = replace_first(o->basePath(), "%T/", "");
        written = false;
        IOHandler->enqueue(IOTask(this, iteration_parameter));
        IOHandler->flush();
        o->iterations.abstractFilePosition = abstractFilePosition;
        o->iterations.written = true;
        parent = &o->iterations;

        /* create iteration path */
        iteration_parameter.path = std::to_string(i);
        abstractFilePosition = std::shared_ptr< AbstractFilePosition >(nullptr);
        written = false;
        IOHandler->enqueue(IOTask(this, iteration_parameter));
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
    /* Find the root point [Output] of this file,
     * meshesPath and particlesPath are stored there */
    Writable *w = this;
    while( w->parent )
        w = w->parent;
    Output* o = dynamic_cast<Output *>(w);

    meshes.flush(o->meshesPath());
    for( auto& m : meshes )
    {
        m.second.flush(m.first);
    }

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

    /* Find the root point [Output] of this file,
     * meshesPath and particlesPath are stored there */
    Writable *w = this;
    while( w->parent )
        w = w->parent;
    Output* o = dynamic_cast<Output *>(w);

    meshes.clear();
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

    particles.clear();
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


