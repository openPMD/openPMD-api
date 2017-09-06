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

public:
    Iteration(Iteration const &);

    template< typename T >
    T time() const;
    template< typename T >
    Iteration& setTime(T);

    template< typename T >
    T dt() const;
    template< typename T >
    Iteration& setDt(T);

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

template< typename T >
inline T
Iteration::time() const
{ return readFloatingpoint< T >("time"); }

template< typename T >
inline T
Iteration::dt() const
{ return readFloatingpoint< T >("dt"); }
