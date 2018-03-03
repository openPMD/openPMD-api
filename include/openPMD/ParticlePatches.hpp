#pragma once

#include "openPMD/backend/Attributable.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/PatchRecord.hpp"

#include <vector>


namespace openPMD
{
class ParticlePatches : public Container< PatchRecord >
{
    friend class ParticleSpecies;

public:
    mapped_type& operator[](key_type const&) override;
    mapped_type& operator[](key_type&&) override;

    size_t
    numPatches() const
    {
        return m_patchPositions.size();
    }

private:
    void read();

    std::vector< PatchPosition > m_patchPositions;
};  //ParticlePatches
} // openPMD
