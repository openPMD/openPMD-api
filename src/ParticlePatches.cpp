#include "openPMD/ParticlePatches.hpp"


namespace openPMD
{
ParticlePatches::mapped_type&
ParticlePatches::operator[](ParticlePatches::key_type const& key)
{
    if( "numParticles" == key || "numParticlesOffset" ==  key )
        throw std::runtime_error("numParticles and numParticlesOffest are handled by the API.");

    return Container< PatchRecord >::operator[](key);
}

ParticlePatches::mapped_type&
ParticlePatches::operator[](ParticlePatches::key_type&& key)
{
    if( "numParticles" == key || "numParticlesOffset" ==  key )
        throw std::runtime_error("numParticles and numParticlesOffest are handled by the API.");

    return Container< PatchRecord >::operator[](key);
}

void
ParticlePatches::read()
{
    Parameter< Operation::LIST_PATHS > pList;
    IOHandler->enqueue(IOTask(this, pList));
    IOHandler->flush();

    Parameter< Operation::OPEN_PATH > pOpen;
    for( auto const& record_name : *pList.paths )
    {
        PatchRecord& pr = (*this)[record_name];
        pOpen.path = record_name;
        IOHandler->enqueue(IOTask(&pr, pOpen));
        IOHandler->flush();
        pr.read();
    }

    Parameter< Operation::LIST_DATASETS > dList;
    IOHandler->enqueue(IOTask(this, dList));
    IOHandler->flush();

    std::vector< uint64_t > numParticles, numParticlesOffset;
    Parameter< Operation::OPEN_DATASET > dOpen;
    Parameter< Operation::READ_DATASET > dRead;
    for( auto const& component_name : *dList.datasets )
    {
        if( !("numParticles" == component_name || "numParticlesOffset" == component_name) )
            throw std::runtime_error("Unexpected record component" + component_name + "in particlePatch");

        PatchRecord& pr = Container< PatchRecord >::operator[](component_name);
        PatchRecordComponent& prc = pr[RecordComponent::SCALAR];
        prc.parent = pr.parent;
        dOpen.name = component_name;
        IOHandler->enqueue(IOTask(&pr, dOpen));
        IOHandler->flush();

        using DT = Datatype;
        if( DT::UINT64 != *dOpen.dtype )
            throw std::runtime_error("Unexpected datatype for " + component_name);

        dRead.dtype = Datatype::UINT64;
        dRead.extent = *dOpen.extent;
        dRead.offset = {0};

        size_t numPoints = dRead.extent[0];
        std::unique_ptr< uint64_t > data(new uint64_t[numPoints]);
        dRead.data = data.get();
        IOHandler->enqueue(IOTask(&pr, dRead));
        IOHandler->flush();

        uint64_t *raw_ptr = static_cast< uint64_t* >(data.get());
        if( "numParticles" == component_name )
        {
            numParticles.reserve(numPoints);
            for( size_t i = 0; i < numPoints; ++i )
                numParticles.emplace_back(*(raw_ptr + i));
        } else if( "numParticlesOffset" == component_name )
        {
            numParticlesOffset.reserve(numPoints);
            for( size_t i = 0; i < numPoints; ++i )
                numParticlesOffset.emplace_back(*(raw_ptr + i));
        }
    }

    if( numParticles.size() != numParticlesOffset.size() )
        throw std::runtime_error("Number of entries in numParticles and numParticlesOffset are different.");

    m_patchPositions.reserve(numParticles.size());
    for( size_t i = 0; i < numParticles.size(); ++i )
        m_patchPositions.emplace_back(PatchPosition(numParticles[i], numParticlesOffset[i]));
}
} // openPMD
