#pragma once

#include <map>
#include <string>

namespace openPMD
{
class Series;
class Iteration;
class Mesh;
class ParticleSpecies;
class PatchRecord;
class PatchRecordComponent;
class RecordComponent;
class MeshRecordComponent;
template <typename, typename, typename>
class Container;
template <typename Val>
using Cont = Container<Val, std::string, std::map<std::string, Val>>;
template <typename>
class BaseRecord;

class HierarchyVisitor
{
public:
    virtual void operator()(Series &) = 0;
    virtual void operator()(Iteration &) = 0;
    virtual void operator()(Cont<Iteration> &) = 0;
    virtual void operator()(Cont<Mesh> &) = 0;
    virtual void operator()(Cont<ParticleSpecies> &) = 0;
    virtual void operator()(Cont<PatchRecord> &) = 0;
    virtual void operator()(Cont<PatchRecordComponent> &) = 0;
    virtual void operator()(Mesh &) = 0;
    virtual void operator()(ParticleSpecies &) = 0;
    virtual void operator()(RecordComponent &) = 0;
    virtual void operator()(MeshRecordComponent &) = 0;
    virtual void operator()(PatchRecordComponent &) = 0;
};
} // namespace openPMD
