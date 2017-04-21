#include "../include/ParticleSpecies.hpp"


ParticleSpecies::ParticleSpecies()
        : m_numParticlesGlobal{0},
          m_numParticlesLocal{0},
          m_numParticlesLocalOffset{0}
{ }

uint64_t
ParticleSpecies::numParticlesGlobal() const
{
    return m_numParticlesGlobal;
}

ParticleSpecies&
ParticleSpecies::setNumParticlesGlobal(uint64_t npg)
{
    m_numParticlesGlobal = npg;
    return *this;
}

uint64_t
ParticleSpecies::numParticlesLocal() const
{
    return m_numParticlesLocal;
}

ParticleSpecies&
ParticleSpecies::setNumParticlesLocal(uint64_t npl)
{
    m_numParticlesLocal = npl;
    return *this;
}

uint64_t
ParticleSpecies::numParticlesLocalOffset() const
{
    return m_numParticlesLocalOffset;
}

ParticleSpecies&
ParticleSpecies::setNumParticlesLocalOffset(uint64_t nplo)
{
    m_numParticlesLocalOffset = nplo;
    return *this;
}
