#pragma once

#include <cstdint>
#include <map>
#include <string>

namespace openPMD
{
class Series;
class Iteration;
class Mesh;
class ParticleSpecies;
class ParticlePatches;
class PatchRecord;
class PatchRecordComponent;
class RecordComponent;
class MeshRecordComponent;
template <typename, typename, typename>
class Container;
template <typename Val, typename Key = std::string>
using Cont = Container<Val, Key, std::map<Key, Val>>;
template <typename>
class BaseRecord;
class Record;

class HierarchyVisitor
{
public:
    virtual void operator()(Series &) = 0;
    virtual void operator()(Iteration &) = 0;
    virtual void operator()(Cont<Iteration, std::uint64_t> &) = 0;
    virtual void operator()(Cont<Mesh> &) = 0;
    virtual void operator()(Cont<ParticleSpecies> &) = 0;
    virtual void operator()(Cont<ParticlePatches> &) = 0;
    virtual void operator()(Cont<PatchRecord> &) = 0;
    virtual void operator()(Cont<Record> &) = 0;
    virtual void operator()(Record &) = 0;
    // TODO there are too many duplications here, remove most of them
    virtual void operator()(Cont<MeshRecordComponent> &) = 0;
    virtual void operator()(BaseRecord<MeshRecordComponent> &) = 0;
    virtual void operator()(Cont<PatchRecordComponent> &) = 0;
    virtual void operator()(BaseRecord<PatchRecordComponent> &) = 0;
    virtual void operator()(Cont<RecordComponent> &) = 0;
    virtual void operator()(BaseRecord<RecordComponent> &) = 0;
    virtual void operator()(ParticleSpecies &) = 0;
    virtual void operator()(RecordComponent &) = 0;
    virtual void operator()(MeshRecordComponent &) = 0;
    virtual void operator()(PatchRecordComponent &) = 0;
};
} // namespace openPMD
