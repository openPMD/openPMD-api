#pragma once


#include "Attributable.hpp"
#include "Container.hpp"
#include "Record.hpp"


class ParticleSpecies : public Container< Record >
{
    template<
            typename T,
            typename T_key
    >
    friend class Container;

private:
    ParticleSpecies();

    uint64_t m_numParticlesGlobal;
    uint64_t m_numParticlesLocal;
    uint64_t m_numParticlesLocalOffset;

public:
    uint64_t numParticlesGlobal() const;
    ParticleSpecies& setNumParticlesGlobal(uint64_t);
    uint64_t numParticlesLocal() const;
    ParticleSpecies& setNumParticlesLocal(uint64_t);
    uint64_t numParticlesLocalOffset() const;
    ParticleSpecies& setNumParticlesLocalOffset(uint64_t);
};
