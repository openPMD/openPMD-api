#include "openPMD/backend/ScientificDefaults.hpp"
#include "openPMD/Iteration.hpp"
#include "openPMD/Mesh.hpp"
#include "openPMD/ParticleSpecies.hpp"
#include "openPMD/backend/Container.hpp"
#include "openPMD/backend/MeshRecordComponent.hpp"

namespace openPMD::internal
{
template <typename Child>
void ScientificDefaults<Child>::finalize(Access at)
{
    if constexpr (IsContainer_v<Child>)
    {
        using Container_t = AsContainer_t<Child>;
        using value_t = typename Container_t::value_type;
        if constexpr (HasScientificDefaults_v<value_t>)
        {
            for (auto &[_, right] : *this)
            {
                (void)_;
                right.ScientificDefaults<value_t>::finalize(at);
            }
        }
    }
}

template class ScientificDefaults<Iteration>;
template class ScientificDefaults<Mesh>;
template class ScientificDefaults<MeshRecordComponent>;
template class ScientificDefaults<ParticleSpecies>;
} // namespace openPMD::internal
