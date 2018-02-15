#pragma once

#include "backend/PatchRecordComponent.hpp"
#include "BaseRecord.hpp"

#include <unordered_map>
#include <string>


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
