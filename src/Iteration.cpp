#include "../include/Iteration.hpp"


Iteration::Iteration() : meshes{Container< Mesh >()},
                         particles{Container< ParticleSpecies >()}
{
    setTime(0);
    setDt(0);
    setTimeUnitSI(0);
}

Iteration::Iteration(Iteration&&) = default;

Iteration::Iteration(Iteration const& i) : meshes{i.meshes},
                                           particles{i.particles}
{ }

Iteration::Iteration(float time,
                     float dt,
                     double timeUnitSI) : meshes{Container< Mesh >()},
                                          particles{
                                                  Container< ParticleSpecies >()}
{
    setTime(time);
    setDt(dt);
    setTimeUnitSI(timeUnitSI);
}

Iteration&
Iteration::operator=(Iteration&& i)
{
    meshes = std::move(i.meshes);
    particles = std::move(i.particles);
    return *this;
}

Iteration&
Iteration::operator=(Iteration const& i)
{
    if( this != &i )
    {
        using std::swap;
        Iteration tmp(i);
        swap(*this, tmp);
    }
    return *this;
}

Iteration::~Iteration()
{ }

float
Iteration::time() const
{
    return boost::get< float >(getAttribute("time"));
}

Iteration
Iteration::setTime(float time)
{
    setAttribute("time", time);
    return *this;
}

float
Iteration::dt() const
{
    return boost::get< float >(getAttribute("dt"));
}

Iteration
Iteration::setDt(float dt)
{
    setAttribute("dt", dt);
    return *this;
}

double
Iteration::timeUnitSI() const
{
    return boost::get< double >(getAttribute("timeUnitSI"));
}

Iteration
Iteration::setTimeUnitSI(double timeUnitSI)
{
    setAttribute("timeUnitSI", timeUnitSI);
    return *this;
}


