#include <memory>

#include "../include/Auxiliary.hpp"
#include "../include/Dataset.hpp"
#include "../include/Iteration.hpp"
#include "../include/Output.hpp"


Iteration::Iteration()
        : meshes{Container< Mesh >()},
          particles{Container< ParticleSpecies >()}
{
    setTime(0);
    setDt(1);
    setTimeUnitSI(1);
}

Iteration::Iteration(float time,
                     float dt,
                     double timeUnitSI)
        : meshes{Container< Mesh >()},
          particles{Container< ParticleSpecies >()}
{
    setTime(time);
    setDt(dt);
    setTimeUnitSI(timeUnitSI);
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

float
Iteration::time() const
{
    return getAttribute("time").get< float >();
}

Iteration&
Iteration::setTime(float time)
{
    setAttribute("time", time);
    dirty = true;
    return *this;
}

float
Iteration::dt() const
{
    return getAttribute("dt").get< float >();
}

Iteration&
Iteration::setDt(float dt)
{
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
    using DT = Datatype;
    Parameter< Operation::READ_ATT > attribute_parameter;

    attribute_parameter.name = "dt";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::FLOAT )
        setDt(Attribute(*attribute_parameter.resource).get< float >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'dt'");

    attribute_parameter.name = "time";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::FLOAT )
        setTime(Attribute(*attribute_parameter.resource).get< float >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'time'");

    attribute_parameter.name = "timeUnitSI";
    IOHandler->enqueue(IOTask(this, attribute_parameter));
    IOHandler->flush();
    if( *attribute_parameter.dtype == DT::DOUBLE )
        setTimeUnitSI(Attribute(*attribute_parameter.resource).get< double >());
    else
        throw std::runtime_error("Unexpected Attribute datatype for 'timeUnitSI'");

    readAttributes();

    /* Find the root point [Output] of this file,
     * meshesPath and particlesPath are stored there */
    Writable *w = this;
    while( w->parent )
        w = w->parent;
    Output* o = dynamic_cast<Output *>(w);

    Parameter< Operation::OPEN_PATH > path_parameter;
    path_parameter.path = o->meshesPath();
    IOHandler->enqueue(IOTask(&meshes, path_parameter));
    IOHandler->flush();

    meshes.readAttributes();

    /* obtain all meshes */
    Parameter< Operation::LIST_PATHS > plist_parameter;
    IOHandler->enqueue(IOTask(&meshes, plist_parameter));
    IOHandler->flush();

    for( auto const& me : *plist_parameter.paths )
    {
        //TODO these may be constant scalar records
        Mesh& m = meshes[me];
        path_parameter.path = me;
        IOHandler->enqueue(IOTask(&m, path_parameter));
        IOHandler->flush();
        m.read();
    }

    Parameter< Operation::LIST_DATASETS > dlist_parameter;
    IOHandler->enqueue(IOTask(&meshes, plist_parameter));
    IOHandler->flush();

    Parameter< Operation::OPEN_DATASET > dataset_parameter;
    for( auto const& me : *dlist_parameter.datasets )
    {
        Mesh& m = meshes[me];
        dataset_parameter.name = me;
        IOHandler->enqueue(IOTask(&m, dataset_parameter));
        IOHandler->flush();
        m.read();
    }

    path_parameter.path = o->particlesPath();
    IOHandler->enqueue(IOTask(&particles, path_parameter));
    IOHandler->flush();

    particles.readAttributes();

    /* obtain all particle species */
    IOHandler->enqueue(IOTask(&particles, plist_parameter));
    IOHandler->flush();

    for( auto const& ps : *plist_parameter.paths )
    {
        ParticleSpecies& p = particles[ps];
        path_parameter.path = ps;
        IOHandler->enqueue(IOTask(&p, path_parameter));
        IOHandler->flush();
        p.read();
    }
}


