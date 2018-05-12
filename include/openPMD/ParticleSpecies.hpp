#pragma once

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/ParticlePatches.hpp"
#include "openPMD/Record.hpp"

#include <string>


namespace openPMD
{

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

namespace traits
{
    template<>
    struct GenerationPolicy< ParticleSpecies >
    {
        template< typename T >
        void operator()(T & ret)
        {
            /* enforce these two RecordComponents as required by the standard */
            ret["position"].setUnitDimension({{UnitDimension::L, 1}});
            ret["positionOffset"].setUnitDimension({{UnitDimension::L, 1}});
            ret.particlePatches.linkHierarchy(ret.m_writable);

            auto& np = ret.particlePatches["numParticles"];
            auto& npc = np[RecordComponent::SCALAR];
            npc.resetDataset(Dataset(Datatype::UINT64, {0}));
            npc.parent = np.parent;
            auto& npo = ret.particlePatches["numParticlesOffset"];
            auto& npoc = npo[RecordComponent::SCALAR];
            npoc.resetDataset(Dataset(Datatype::UINT64, {0}));
            npoc.parent = npo.parent;
        }
    };
} // traits
} // openPMD
