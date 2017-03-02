#pragma once


#include <map>
#include <string>
#include <vector>

#include "Attributable.hpp"
#include "Container.hpp"
#include "Mesh.hpp"

using ParticleSpecies = Container< Record >;

class Iteration : public Attributable
{
public:
    Iteration();
//    Iteration(Iteration&&);
//    Iteration(Iteration const&);
    Iteration(float time, float dt, double timeUnitSI);
//    Iteration& operator=(Iteration&&);
//    Iteration& operator=(Iteration const&);
//    ~Iteration();

    float time() const;
    Iteration& setTime(float);

    float dt() const;
    Iteration& setDt(float);

    double timeUnitSI() const;
    Iteration& setTimeUnitSI(double);

    Container< Mesh > meshes;
    Container< ParticleSpecies > particles; //particleSpecies?
};  //Iteration
