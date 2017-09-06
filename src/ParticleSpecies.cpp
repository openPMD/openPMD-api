#include "../include/Auxiliary.hpp"
#include "../include/ParticleSpecies.hpp"


ParticleSpecies::ParticleSpecies()
        : m_numParticlesGlobal{0},
          m_numParticlesLocal{0},
          m_numParticlesLocalOffset{0}
{ }

uint64_t
ParticleSpecies::numParticlesGlobal() const
{
    return m_numParticlesGlobal;
}

ParticleSpecies&
ParticleSpecies::setNumParticlesGlobal(uint64_t npg)
{
    m_numParticlesGlobal = npg;
    return *this;
}

uint64_t
ParticleSpecies::numParticlesLocal() const
{
    return m_numParticlesLocal;
}

ParticleSpecies&
ParticleSpecies::setNumParticlesLocal(uint64_t npl)
{
    m_numParticlesLocal = npl;
    return *this;
}

uint64_t
ParticleSpecies::numParticlesLocalOffset() const
{
    return m_numParticlesLocalOffset;
}

ParticleSpecies&
ParticleSpecies::setNumParticlesLocalOffset(uint64_t nplo)
{
    m_numParticlesLocalOffset = nplo;
    return *this;
}

void
ParticleSpecies::read()
{
    clear();
    /* obtain all non-scalar records */
    Parameter< Operation::LIST_PATHS > plist_parameter;
    IOHandler->enqueue(IOTask(this, plist_parameter));
    IOHandler->flush();

    Parameter< Operation::OPEN_PATH > path_parameter;
    Parameter< Operation::LIST_ATTS > alist_parameter;
    for( auto const& record_name : *plist_parameter.paths )
    {
        if( strip(record_name, {'\0'}) == "particlePatches" )
            continue;

        Record& r = (*this)[record_name];
        path_parameter.path = record_name;
        alist_parameter.attributes->clear();
        IOHandler->enqueue(IOTask(&r, path_parameter));
        IOHandler->enqueue(IOTask(&r, alist_parameter));
        IOHandler->flush();

        auto begin = alist_parameter.attributes->begin();
        auto end = alist_parameter.attributes->end();
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

    /* obtain all scalar records */
    Parameter< Operation::LIST_DATASETS > dlist_parameter;
    IOHandler->enqueue(IOTask(this, dlist_parameter));
    IOHandler->flush();

    Parameter< Operation::OPEN_DATASET > dataset_parameter;
    for( auto const& record_name : *dlist_parameter.datasets )
    {
        Record& r = (*this)[record_name];
        dataset_parameter.name = record_name;
        IOHandler->enqueue(IOTask(&r, dataset_parameter));
        IOHandler->flush();
        RecordComponent& rc = r[RecordComponent::SCALAR];
        rc.abstractFilePosition = r.abstractFilePosition;
        rc.parent = r.parent;
        rc.written = true;
        rc.resetDataset(Dataset(*dataset_parameter.dtype, *dataset_parameter.extent));
        r.read();
    }

    readAttributes();
}
