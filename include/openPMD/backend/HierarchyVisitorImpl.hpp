#pragma once

#include "openPMD/backend/HierarchyVisitor.hpp"

#include "openPMD/Iteration.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/Series.hpp"
#include <type_traits>

namespace openPMD
{
template <typename Lambda>
class HierarchyVisitorFromLambda : public HierarchyVisitor
{
    Lambda lambda;

    template <typename Arg>
    HierarchyVisitorFromLambda(uint8_t /* constructor_tag */, Arg &&arg)
        : lambda(std::forward<Arg>(arg))
    {}

    template <typename Lambda_in>
    friend auto makeHierarchyVisitorFromLambda(Lambda_in &&lambda);

public:
    void operator()(Iteration &obj) override
    {
        lambda(obj);
    }
    void operator()(Iterations &obj) override
    {
        lambda(obj);
    }
    void operator()(Mesh &obj) override
    {
        lambda(obj);
    }
    void operator()(MeshRecordComponent &obj) override
    {
        lambda(obj);
    }
    void operator()(Meshes &obj) override
    {
        lambda(obj);
    }
    void operator()(ParticlePatches &obj) override
    {
        lambda(obj);
    }
    void operator()(PatchRecord &obj) override
    {
        lambda(obj);
    }
    void operator()(ParticleSpecies &obj) override
    {
        lambda(obj);
    }
    void operator()(Particles &obj) override
    {
        lambda(obj);
    }
    void operator()(PatchRecordComponent &obj) override
    {
        lambda(obj);
    }
    void operator()(Record &obj) override
    {
        lambda(obj);
    }
    void operator()(RecordComponent &obj) override
    {
        lambda(obj);
    }
    void operator()(Series &obj) override
    {
        lambda(obj);
    }
};

template <typename Lambda>
auto makeHierarchyVisitorFromLambda(Lambda &&lambda)
{
    using res_t = HierarchyVisitorFromLambda<std::remove_reference_t<Lambda>>;
    return res_t{0, std::forward<Lambda>(lambda)};
}

template <typename Lambda>
void Attributable::visitHierarchyFromLambda(Lambda &&lambda, bool recursive)
{
    auto visitor = makeHierarchyVisitorFromLambda(std::forward<Lambda>(lambda));
    this->visitHierarchy(visitor, recursive);
}
} // namespace openPMD
