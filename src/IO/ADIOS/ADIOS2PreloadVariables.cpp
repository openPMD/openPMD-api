#include "openPMD/config.hpp"
#if openPMD_HAVE_ADIOS2

#include "openPMD/IO/ADIOS/ADIOS2PreloadVariables.hpp"

#include <algorithm>

namespace openPMD::detail
{
namespace
{
    template <typename Iterator, typename Filter>
    struct FilteredInputIterator : Iterator
    {
        Iterator m_end;
        Filter m_filter;
        FilteredInputIterator(Iterator it, Iterator end, Filter filter)
            : Iterator(std::move(it))
            , m_end(std::move(end))
            , m_filter(std::move(filter))
        {
            if (it != end && !m_filter(*it))
            {
                operator++();
            }
        }
        auto operator++() -> FilteredInputIterator &
        {
            do
            {
                Iterator::operator++();
            } while (*this != m_end && !m_filter(**this));
            return *this;
        }
        auto operator==(FilteredInputIterator const &other) const -> bool
        {
            return static_cast<Iterator const *>(this)->operator==(other);
        }
        using Iterator::operator*;
        using Iterator::operator->;

        [[nodiscard]] auto end() const -> FilteredInputIterator
        {
            return {m_end, m_end, m_filter};
        }
    };
} // namespace

auto AdiosVariables::availableVariables(
    size_t step, bool use_step_selection, adios2::IO &IO)
    -> AttributeMap_t const &
{
    if (!use_step_selection && m_preparsed.has_value())
    {
        // Currently stepped out of step selection, but using ReadRandomAccess
        return m_preparsed->m_allVariables;
    }
    // Shortcut: No need to recompute variables if we find they are static
    if (variables_are_static && m_availableVariables.has_value())
    {
        return *m_availableVariables;
    }
    // Check if variables need to be (re)computed
    if (!m_availableVariables || step != this->currentStep)
    {
        if (m_preparsed.has_value())
        {
            auto &preparsed = *m_preparsed;
            auto begin = FilteredInputIterator(
                preparsed.m_allVariables.begin(),
                preparsed.m_allVariables.end(),
                [&](auto const &pair) {
                    auto partial =
                        preparsed.m_partialVariables.find(pair.first);
                    if (partial == preparsed.m_partialVariables.end())
                    {
                        // Variable is available in all steps
                        return true;
                    }
                    auto &defined_steps = partial->second;
                    return std::find(
                               defined_steps.begin(),
                               defined_steps.end(),
                               step) != defined_steps.end();
                });
            m_availableVariables = {begin, begin.end()};
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

#endif
