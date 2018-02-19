#include "openPMD/auxiliary/StringManip.hpp"
#include "openPMD/ParticleSpecies.hpp"


ParticleSpecies::ParticleSpecies() = default;

void
ParticleSpecies::read()
{
    /* allow all attributes to be set */
    written = false;

    clear_unchecked();

    /* obtain all non-scalar records */
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler->enqueue(IOTask(this, pList));
    IOHandler->flush();

    Parameter< Operation::OPEN_PATH > pOpen;
    Parameter< Operation::LIST_ATTS > aList;
    for( auto const& record_name : *pList.paths )
    {
        if( record_name == "particlePatches" )
        {
            pOpen.path = "particlePatches";
            IOHandler->enqueue(IOTask(&particlePatches, pOpen));
            IOHandler->flush();
            particlePatches.read();
        } else
        {
            Record& r = (*this)[record_name];
            pOpen.path = record_name;
            aList.attributes->clear();
            IOHandler->enqueue(IOTask(&r, pOpen));
            IOHandler->enqueue(IOTask(&r, aList));
            IOHandler->flush();

            auto begin = aList.attributes->begin();
            auto end = aList.attributes->end();
            auto value = std::find(begin, end, "value");
            auto shape = std::find(begin, end, "shape");
            if( value != end && shape != end )
            {
                RecordComponent& rc = r[RecordComponent::SCALAR];
                rc.m_isConstant = true;
                rc.parent = r.parent;
                rc.abstractFilePosition = r.abstractFilePosition;
            }
            r.read();
        }
    }

    /* obtain all scalar records */
    Parameter< Operation::LIST_DATASETS > dList;
    IOHandler->enqueue(IOTask(this, dList));
    IOHandler->flush();

    Parameter< Operation::OPEN_DATASET > dOpen;
    for( auto const& record_name : *dList.datasets )
    {
        Record& r = (*this)[record_name];
        dOpen.name = record_name;
        IOHandler->enqueue(IOTask(&r, dOpen));
        IOHandler->flush();
        RecordComponent& rc = r[RecordComponent::SCALAR];
        rc.abstractFilePosition = r.abstractFilePosition;
        rc.parent = r.parent;
        rc.written = false;
        rc.resetDataset(Dataset(*dOpen.dtype, *dOpen.extent));
        rc.written = true;
        r.read();
    }

    readAttributes();

    /* this file need not be flushed */
    written = true;
}

void
ParticleSpecies::flush(std::string const& path)
{
    Container< Record >::flush(path);

    for( auto& record : *this )
        record.second.flush(record.first);

    particlePatches.flush("particlePatches");
    for( auto& patch : particlePatches )
        patch.second.flush(patch.first);
}

template<>
Container< ParticleSpecies >::mapped_type&
Container< ParticleSpecies >::operator[](Container< ParticleSpecies >::key_type const& key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        ParticleSpecies ps = ParticleSpecies();
        ps.IOHandler = IOHandler;
        ps.parent = this;
        auto& ret = this->insert({key, ps}).first->second;
        /* enforce these two RecordComponents as required by the standard */
        ret["position"].setUnitDimension({{UnitDimension::L, 1}});
        ret["positionOffset"].setUnitDimension({{UnitDimension::L, 1}});
        ret.particlePatches.parent = &ret;
        ret.particlePatches.IOHandler = ret.IOHandler;
        return ret;
    }
}

template<>
Container< ParticleSpecies >::mapped_type&
Container< ParticleSpecies >::operator[](Container< ParticleSpecies >::key_type && key)
{
    auto it = this->find(key);
    if( it != this->end() )
        return it->second;
    else
    {
        ParticleSpecies ps = ParticleSpecies();
        ps.IOHandler = IOHandler;
        ps.parent = this;
        auto& ret = this->insert({std::move(key), ps}).first->second;
        /* enforce these two RecordComponents as required by the standard */
        ret["position"].setUnitDimension({{UnitDimension::L, 1}});
        ret["positionOffset"].setUnitDimension({{UnitDimension::L, 1}});
        ret.particlePatches.parent = &ret;
        ret.particlePatches.IOHandler = ret.IOHandler;
        return ret;
    }
}
