#pragma once


#include <map>
#include <string>
#include <vector>

#include "Attributable.hpp"
#include "Container.hpp"
#include "Mesh.hpp"
#include "ParticleSpecies.hpp"


class Iteration : public Attributable
{
public:
    Iteration();
    Iteration(float time, float dt, double timeUnitSI);

    float time() const;
    Iteration& setTime(float);

    float dt() const;
    Iteration& setDt(float);

    double timeUnitSI() const;
    Iteration& setTimeUnitSI(double);

    Container< Mesh > meshes;
    Container< ParticleSpecies > particles; //particleSpecies?
};  //Iteration
