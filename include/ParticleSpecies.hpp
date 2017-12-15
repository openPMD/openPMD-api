#pragma once


#include "backend/Attributable.hpp"
#include "backend/Container.hpp"
#include "ParticlePatches.hpp"
#include "Record.hpp"


class ParticleSpecies : public Container< Record >
{
    friend class Container< ParticleSpecies >;
    friend class Container< Record >;
    friend class Iteration;

public:
    ParticlePatches particlePatches;

private:
    ParticleSpecies();

    void read();
    void flush(std::string const &) override;
};

template<>
Container< ParticleSpecies >::mapped_type&
Container< ParticleSpecies >::operator[](Container< ParticleSpecies >::key_type const& key);

template<>
Container< ParticleSpecies >::mapped_type&
Container< ParticleSpecies >::operator[](Container< ParticleSpecies >::key_type && key);