#include "openPMD/IO/ADIOS/ADIOS2PreloadVariables.hpp"
#include <stdexcept>

namespace openPMD::detail
{
auto AdiosVariables::availableAttributes(size_t step, adios2::IO &IO)
    -> AttributeMap_t const &
{
    if (!m_availableVariables || step != this->currentStep)
    {
        if (m_preparsed.has_value())
        {
            throw std::runtime_error("Unimplemented!");
        }
        else
        {
            m_availableVariables = IO.AvailableVariables();
        }
        this->currentStep = step;
    }
    return *m_availableVariables;
}
} // namespace openPMD::detail
