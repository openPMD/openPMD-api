#include "../include/Iteration.hpp"


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

float
Iteration::time() const
{
    return boost::get< float >(getAttribute("time").getResource());
}

Iteration&
Iteration::setTime(float time)
{
    setAttribute("time", time);
    return *this;
}

float
Iteration::dt() const
{
    return boost::get< float >(getAttribute("dt").getResource());
}

Iteration&
Iteration::setDt(float dt)
{
    setAttribute("dt", dt);
    return *this;
}

double
Iteration::timeUnitSI() const
{
    return boost::get< double >(getAttribute("timeUnitSI").getResource());
}

Iteration&
Iteration::setTimeUnitSI(double timeUnitSI)
{
    setAttribute("timeUnitSI", timeUnitSI);
    return *this;
}


