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
    template<
            typename T,
            typename T_key
    >
    friend class Container;
    friend class Output;

private:
    Iteration();
    Iteration(float time, float dt, double timeUnitSI);

public:
    Iteration(Iteration const &);

    float time() const;
    Iteration& setTime(float);

    float dt() const;
    Iteration& setDt(float);

    double timeUnitSI() const;
    Iteration& setTimeUnitSI(double);

    Container< Mesh > meshes;
    Container< ParticleSpecies > particles; //particleSpecies?

private:
    void flushFileBased(uint64_t);
    void flushGroupBased(uint64_t);
    void flush();
    void read();
};  //Iteration
