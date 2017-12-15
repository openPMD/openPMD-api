#pragma once


#include <cstdint>


struct PatchPosition
{
    PatchPosition()
            : numParticles{0},
              numParticlesOffset{0}
    { }
    PatchPosition(uint64_t numParticles, uint64_t numParticlesOffset)
            : numParticles{numParticles},
              numParticlesOffset{numParticlesOffset}
    { }

    uint64_t numParticles;
    uint64_t numParticlesOffset;
};  //PatchPosition

inline bool
operator==(PatchPosition const& pp1, PatchPosition const& pp2)
{
    return (pp1.numParticles == pp2.numParticles)
           && (pp1.numParticlesOffset == pp2.numParticlesOffset);
}

namespace std
{
    template<>
    struct hash< PatchPosition >
    {
        std::size_t operator()(const PatchPosition& k) const
        {
            using std::hash;

            return hash< uint64_t >()(k.numParticles)
                   ^ (hash< uint64_t >()(k.numParticlesOffset)<<1);
        }
    };
}