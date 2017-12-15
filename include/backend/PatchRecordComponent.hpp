#pragma once


#include <unordered_map>

#include "backend/BaseRecordComponent.hpp"
#include "backend/GenericPatchData.hpp"
#include "backend/PatchPosition.hpp"


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
