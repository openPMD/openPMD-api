#include <memory>

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
Iteration::flush()
{
    if( dirty )
    {
        Parameter< Operation::WRITE_ATT > attribute_parameter;
        for( std::string const & att_name : attributes() )
        {
            attribute_parameter.name = att_name;
            attribute_parameter.resource = getAttribute(att_name).getResource();
            attribute_parameter.dtype = getAttribute(att_name).dtype;
            IOHandler->enqueue(IOTask(this, attribute_parameter));
        }
    }

    /* Find the root point [Output] of this file,
     * meshesPath and particlesPath are stored there */
    Writable *w = this;
    while( w->parent )
        w = w->parent;

    Output* o = dynamic_cast<Output *>(w);
    Parameter< Operation::CREATE_PATH > path_parameter;
    /* Create the meshesPath */
    if( !meshes.written )
    {
        path_parameter.path = o->getAttribute("meshesPath").get< std::string >();
        IOHandler->enqueue(IOTask(&meshes, path_parameter));
    }
    meshes.flush();
    for( auto& m : meshes )
    {
        /* Create the meshes path/dataset */
        if( !m.second.written )
        {
            if( m.second.m_containsScalar )
            {
                Parameter< Operation::CREATE_DATASET > ds_parameter;
                ds_parameter.name = m.first;
                Dataset const& ds = m.second.at(RecordComponent::SCALAR).m_dataset;
                ds_parameter.dtype = ds.dtype;
                ds_parameter.extent = ds.extents;
                IOHandler->enqueue(IOTask(&m.second, ds_parameter));
            } else
            {
                path_parameter.path = m.first;
                IOHandler->enqueue(IOTask(&m.second, path_parameter));
            }
        }
        m.second.flush();
    }

    /* Create the particlesPath */
    if( !particles.written )
    {
        path_parameter.path = o->getAttribute("particlesPath").get< std::string >();
        IOHandler->enqueue(IOTask(&particles, path_parameter));
    }
    particles.flush();
    for( auto& spec : particles )
    {
        /* Create a path to the particle species */
        if( !spec.second.written )
        {
            path_parameter.path = spec.first;
            IOHandler->enqueue(IOTask(&spec.second, path_parameter));
        }
        spec.second.flush();
        for( auto& rec : spec.second )
        {
            /* Create the particle record's path/dataset */
            if( !rec.second.written )
            {
                if( rec.second.m_containsScalar )
                {
                    Parameter< Operation::CREATE_DATASET > ds_parameter;
                    ds_parameter.name = rec.first;
                    Dataset const& ds = rec.second[RecordComponent::SCALAR].m_dataset;
                    ds_parameter.dtype = ds.dtype;
                    ds_parameter.extent = ds.extents;
                    IOHandler->enqueue(IOTask(&rec.second, ds_parameter));
                } else
                {
                    path_parameter.path = rec.first;
                    IOHandler->enqueue(IOTask(&rec.second, path_parameter));
                }
            }
            rec.second.flush();
        }
    }

    dirty = false;
}


