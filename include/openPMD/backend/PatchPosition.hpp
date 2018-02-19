#pragma once

#include <cstdint>
#include <functional>


namespace openPMD
{
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

    bool
    operator==(openPMD::PatchPosition const & other) const
    {
        return (this->numParticles == other.numParticles)
               && (this->numParticlesOffset == other.numParticlesOffset);
    }

    uint64_t numParticles;
    uint64_t numParticlesOffset;
};
} // openPMD

namespace std
{
    template<>
    struct hash< openPMD::PatchPosition >
    {
        std::size_t
        operator()(openPMD::PatchPosition const & k) const
        {
            using std::hash;

            //! @todo add comment about this hash
            return hash< uint64_t >()(k.numParticles)
                   ^ (hash< uint64_t >()(k.numParticlesOffset) << 1);
        }
    };
}

