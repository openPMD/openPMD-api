#pragma once

namespace openPMD
{
class Iteration;
class Iterations;
class Mesh;
class MeshRecordComponent;
class Meshes;
class ParticlePatches;
class ParticleSpecies;
class Particles;
class PatchRecord;
class PatchRecordComponent;
class Record;
class RecordComponent;
class Series;

class HierarchyVisitor
{
public:
    virtual void operator()(Iteration &) = 0;
    virtual void operator()(Iterations &) = 0;
    virtual void operator()(Mesh &) = 0;
    virtual void operator()(MeshRecordComponent &) = 0;
    virtual void operator()(Meshes &) = 0;
    virtual void operator()(ParticlePatches &) = 0;
    virtual void operator()(ParticleSpecies &) = 0;
    virtual void operator()(Particles &) = 0;
    virtual void operator()(PatchRecord &) = 0;
    virtual void operator()(PatchRecordComponent &) = 0;
    virtual void operator()(Record &) = 0;
    virtual void operator()(RecordComponent &) = 0;
    virtual void operator()(Series &) = 0;
};
} // namespace openPMD
