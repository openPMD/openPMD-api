#pragma once

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/Record.hpp"

#include <string>


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
