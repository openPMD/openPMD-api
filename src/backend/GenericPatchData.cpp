#include "openPMD/backend/GenericPatchData.hpp"

namespace openPMD
{
GenericPatchData::GenericPatchData()
        : m_data(0)
{
    m_data.dtype = Dtype::UNDEFINED;
}
} // openPMD

