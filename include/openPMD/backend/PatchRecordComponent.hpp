#pragma once

#include "openPMD/backend/BaseRecordComponent.hpp"
#include "openPMD/backend/GenericPatchData.hpp"
#include "openPMD/backend/PatchPosition.hpp"

#include <unordered_map>
#include <string>


class PatchRecordComponent : public BaseRecordComponent
{
    template<
            typename T,
            typename T_key,
            typename T_container
    >
    friend
    class Container;

    friend class ParticlePatches;
    friend class PatchRecord;

public:
    GenericPatchData& operator[](PatchPosition const&);

    PatchRecordComponent& setUnitSI(double);

private:
    PatchRecordComponent();

    void flush(std::string const&);

    std::unordered_map< PatchPosition, GenericPatchData > m_data;
};  //PatchRecordComponent
