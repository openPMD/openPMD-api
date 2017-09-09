#pragma once


#include "Attributable.hpp"
#include "Container.hpp"
#include "Record.hpp"


class ParticleSpecies : public Container< Record >
{
    friend class Container< ParticleSpecies >;
    friend class Container< Record >;
    friend class Iteration;

private:
    ParticleSpecies();

    uint64_t m_numParticlesGlobal;
    uint64_t m_numParticlesLocal;
    uint64_t m_numParticlesLocalOffset;

    void read();

public:
    uint64_t numParticlesGlobal() const;
    ParticleSpecies& setNumParticlesGlobal(uint64_t);
    uint64_t numParticlesLocal() const;
    ParticleSpecies& setNumParticlesLocal(uint64_t);
    uint64_t numParticlesLocalOffset() const;
    ParticleSpecies& setNumParticlesLocalOffset(uint64_t);
};

template<>
inline typename Container< ParticleSpecies >::mapped_type&
Container< ParticleSpecies >::operator[](Container< ParticleSpecies >::key_type key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        ParticleSpecies ps = ParticleSpecies();
        ps.IOHandler = IOHandler;
        ps.parent = this;
        auto& ret = this->insert({key, std::move(ps)}).first->second;
        /* enforce these two RecordComponents as required by the standard */
        ret["position"].setUnitDimension({{Record::UnitDimension::L, 1}});
        ret["positionOffset"].setUnitDimension({{Record::UnitDimension::L, 1}});
        return ret;
    }
}