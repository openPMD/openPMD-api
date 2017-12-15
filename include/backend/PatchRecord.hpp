#pragma once


#include <unordered_map>

#include "backend/PatchRecordComponent.hpp"
#include "BaseRecord.hpp"


class PatchRecord : public BaseRecord< PatchRecordComponent >
{
    friend class Container< PatchRecord >;
    friend class ParticleSpecies;
    friend class ParticlePatches;

public:
    PatchRecord& setUnitDimension(std::map< UnitDimension, double > const&);

private:
    PatchRecord();

    void flush(std::string const&) override;
    void read() override;
};  //PatchRecord
