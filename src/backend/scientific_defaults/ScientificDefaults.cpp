#include "openPMD/backend/scientific_defaults/ScientificDefaults.hpp"
#include "openPMD/backend/scientific_defaults/ConfigAttribute.hpp"

#include "openPMD/IO/AbstractIOHandler.hpp"
#include "openPMD/backend/Writable.hpp"
#include "openPMD/backend/scientific_defaults/ScientificDefaults_auxiliary.hpp"

namespace openPMD::internal
{
// 7. ScientificDefaults template implementations
[[nodiscard]] auto
ScientificDefaults::defaultAttribute(Attributable &attr, char const *attrName)
    -> ConfigAttribute
{
    return ConfigAttribute{attr, attrName};
}

void ScientificDefaults::writeDefaults(OpenpmdStandard standard)
{
    scientificDefaults_impl(WriteOrRead::Write, standard);
}

void ScientificDefaults::readDefaults(
    Attributable &self, OpenpmdStandard standard)
{
    auto old_dirty = self.dirty();
    scientificDefaults_impl(WriteOrRead::Read, standard);
    if (!old_dirty)
    {
        // did not become dirty by reading
        // but setAttribute() might have been used, so set it again
        self.setDirty(false);
    }
}
} // namespace openPMD::internal
