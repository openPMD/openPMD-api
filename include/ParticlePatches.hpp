#pragma once


#include "backend/Attributable.hpp"
#include "backend/Container.hpp"
#include "backend/PatchRecord.hpp"


class ParticlePatches : public Container< PatchRecord >
{
    friend class ParticleSpecies;

public:
    mapped_type& operator[](key_type const&) override;
    mapped_type& operator[](key_type&&) override;

private:
    void read();

    std::vector< PatchPosition > m_patchPositions;
};  //ParticlePatches
