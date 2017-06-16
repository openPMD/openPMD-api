#include <memory>

#include "../include/Iteration.hpp"
#include "../include/Output.hpp"


Iteration::Iteration()
        : meshes{Container< Mesh >()},
          particles{Container< ParticleSpecies >()}
{
    setTime(0);
    setDt(0);
    setTimeUnitSI(0);
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

    Writable *w = this;
    while( w->parent )
        w = w->parent;

    Output* o = dynamic_cast<Output *>(w);
    std::string mp = o->getAttribute("meshesPath").get< std::string >();
    std::string pp = o->getAttribute("particlesPath").get< std::string >();
    Parameter< Operation::CREATE_PATH > parameter;
    if( !meshes.written )
    {
        parameter.path = mp;
        IOHandler->enqueue(IOTask(&meshes, parameter));
    }
    meshes.flush();
    for( auto& m : meshes )
    {
        if( !m.second.written )
        {
            parameter.path = m.first;
            //TODO this might be a scalar, i.e. might not require a group
            IOHandler->enqueue(IOTask(&m.second, parameter));
        }
        m.second.flush();
    }

    if( !particles.written )
    {
        parameter.path = pp;
        IOHandler->enqueue(IOTask(&particles, parameter));
    }
    particles.flush();
    for( auto& p : particles )
    {
        if( !p.second.written )
        {
            parameter.path = p.first;
            //TODO this might be a scalar, i.e. might not require a group
            IOHandler->enqueue(IOTask(&p.second, parameter));
        }
        p.second.flush();
    }

    dirty = false;
}


