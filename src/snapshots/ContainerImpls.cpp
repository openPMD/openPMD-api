#include "openPMD/snapshots/ContainerImpls.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include <memory>
#include <optional>
#include <stdexcept>

namespace openPMD
{
namespace
{
    using value_type =
        Container<Iteration, Iteration::IterationIndex_t>::value_type;
    auto stateful_to_opaque(StatefulIterator const &it)
        -> OpaqueSeriesIterator<value_type>
    {
        std::unique_ptr<DynamicSeriesIterator<value_type>>
            internal_iterator_cloned{new StatefulIterator(it)};
        return OpaqueSeriesIterator<value_type>(
            std::move(internal_iterator_cloned));
    }
} // namespace

StatefulSnapshotsContainer::StatefulSnapshotsContainer(
    std::function<StatefulIterator *()> begin)
    : m_begin(std::move(begin))
{}
auto StatefulSnapshotsContainer::get() -> StatefulIterator *
{
    if (!m_bufferedIterator.has_value())
    {
        m_bufferedIterator = m_begin();
    }
    return *m_bufferedIterator;
}
auto StatefulSnapshotsContainer::get() const -> StatefulIterator const *
{
    return m_bufferedIterator.value_or(nullptr);
}
auto StatefulSnapshotsContainer::currentIteration()
    -> std::optional<value_type *>
{
    if (auto it = get(); it)
    {
        return it->peekCurrentlyOpenIteration();
    }
    else
    {
        return nullptr;
    }
}
auto StatefulSnapshotsContainer::currentIteration() const
    -> std::optional<value_type const *>
{
    if (auto it = get(); it)
    {
        return it->peekCurrentlyOpenIteration();
    }
    else
    {
        return nullptr;
    }
}
auto StatefulSnapshotsContainer::begin() -> iterator
{
    return stateful_to_opaque(*get());
}
auto StatefulSnapshotsContainer::end() -> iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new StatefulIterator()});
}
auto StatefulSnapshotsContainer::begin() const -> const_iterator
{
    throw error::WrongAPIUsage(
        "[StatefulSnapshotsContainer::begin] Const iteration not possible on a "
        "stateful container/iterator.");
}
auto StatefulSnapshotsContainer::end() const -> const_iterator
{
    throw error::WrongAPIUsage(
        "[StatefulSnapshotsContainer::end] Const iteration not possible on a "
        "stateful container/iterator.");
}
auto StatefulSnapshotsContainer::rbegin() -> iterator
{
    /*
     * @todo maybe can adapt std::reverse_iterator as soon as the stateful
     * iterator is powerful enough for this
     */
    throw std::runtime_error(
        "Reverse iteration not (yet) implemented on a stateful "
        "container/iterator.");
}
auto StatefulSnapshotsContainer::rend() -> iterator
{
    throw std::runtime_error(
        "Reverse iteration not (yet) implemented on a stateful "
        "container/iterator.");
}
auto StatefulSnapshotsContainer::rbegin() const -> const_iterator
{
    throw error::WrongAPIUsage(
        "[StatefulSnapshotsContainer::rbegin] Const iteration not possible on "
        "a stateful container/iterator.");
}
auto StatefulSnapshotsContainer::rend() const -> const_iterator
{
    throw error::WrongAPIUsage(
        "[StatefulSnapshotsContainer::rend] Const iteration not possible on a "
        "stateful container/iterator.");
}

bool StatefulSnapshotsContainer::empty() const
{
    auto it = get();
    return (!it) || !it->operator bool();
}
auto StatefulSnapshotsContainer::size() const -> size_t
{
    throw std::runtime_error("Unimplemented");
}

auto StatefulSnapshotsContainer::at(key_type const &) const
    -> mapped_type const &
{
    throw std::runtime_error(
        "Item access not (yet) implemented on a stateful "
        "container/iterator.");
}
auto StatefulSnapshotsContainer::at(key_type const &) -> mapped_type &
{
    throw std::runtime_error(
        "Item access not (yet) implemented on a stateful "
        "container/iterator.");
}

auto StatefulSnapshotsContainer::operator[](key_type const &key)
    -> mapped_type &
{
    auto base_iterator = get();
    auto &shared = base_iterator->m_data;
    if (!shared || !shared->has_value())
    {
        throw error::WrongAPIUsage(
            "[WriteIterations] Trying to access after closing Series.");
    }
    auto &s = shared->value();
    auto access = s.series.IOHandler()->m_frontendAccess;

    // @todo distinguish read_write
    if (access == Access::READ_WRITE)
    {
        throw std::runtime_error(
            "[StatefulSnapshotsContainer::operator[]()] Unimplemented for "
            "READ_WRITE mode.");
    }
    if (access::write(access))
    {
        auto lastIteration = base_iterator->peekCurrentlyOpenIteration();
        if (lastIteration.has_value())
        {
            auto lastIteration_v = lastIteration.value();
            if (lastIteration_v->first == key)
            {
                return s.series.iterations.at(key);
            }
            else
            {
                lastIteration_v->second.close(); // continue below
            }
        }
        if (auto it = s.series.iterations.find(key);
            it == s.series.iterations.end())
        {
            s.currentStep.map_during_t(
                [&](detail::CurrentStep::During_t &during) {
                    ++during.idx;
                    base_iterator->get().seen_iterations[key] = during.idx;
                    during.iteration_idx = key;
                    during.available_iterations_in_step = {key};
                },
                [&](detail::CurrentStep::AtTheEdge where_am_i)
                    -> detail::CurrentStep::During_t {
                    base_iterator->get().seen_iterations[key] = 0;
                    switch (where_am_i)
                    {
                    case detail::CurrentStep::AtTheEdge::Begin:
                        return detail::CurrentStep::During_t{0, key, {key}};
                    case detail::CurrentStep::AtTheEdge::End:
                        throw error::Internal(
                            "Trying to create a new output step, but the "
                            "stream is "
                            "closed?");
                    }
                    throw std::runtime_error("Unreachable!");
                });
        }
        auto &res = s.series.iterations[key];
        if (res.getStepStatus() != StepStatus::DuringStep)
        {
            try
            {
                res.beginStep(/* reread = */ false);
            }
            catch (error::OperationUnsupportedInBackend const &)
            {
                s.series.iterations.retrieveSeries()
                    .get()
                    .m_currentlyActiveIterations.clear();
                throw;
            }
            res.setStepStatus(StepStatus::DuringStep);
        }
        return res;
    }
    else if (access::read(access))
    {
        auto result = base_iterator->seek(
            {StatefulIterator::Seek::Seek_Iteration_t{key}});
        return (*result)->second;
    }
    throw error::Internal("Control flow error: This should be unreachable.");
}

auto StatefulSnapshotsContainer::clear() -> void
{
    throw std::runtime_error("Unimplemented");
}

auto StatefulSnapshotsContainer::find(key_type const &) -> iterator
{
    throw std::runtime_error("Unimplemented");
}
auto StatefulSnapshotsContainer::find(key_type const &) const -> const_iterator
{
    throw error::WrongAPIUsage(
        "[StatefulSnapshotsContainer::find] Const iteration not possible on a "
        "stateful container/iterator.");
}

auto StatefulSnapshotsContainer::contains(key_type const &) const -> bool
{
    throw std::runtime_error("Unimplemented");
}

RandomAccessIteratorContainer::RandomAccessIteratorContainer(
    Container<Iteration, key_type> cont)
    : m_cont(std::move(cont))
{}
auto RandomAccessIteratorContainer::begin() -> iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.begin())});
}
auto RandomAccessIteratorContainer::end() -> iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.end())});
}
auto RandomAccessIteratorContainer::begin() const -> const_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.begin())});
}
auto RandomAccessIteratorContainer::end() const -> const_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.end())});
}
auto RandomAccessIteratorContainer::rbegin() -> reverse_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.rbegin())});
}
auto RandomAccessIteratorContainer::rend() -> reverse_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.end())});
}
auto RandomAccessIteratorContainer::rbegin() const -> const_reverse_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.rbegin())});
}
auto RandomAccessIteratorContainer::rend() const -> const_reverse_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.rend())});
}

auto RandomAccessIteratorContainer::empty() const -> bool
{
    return m_cont.empty();
}
auto RandomAccessIteratorContainer::size() const -> size_t
{
    return m_cont.size();
}

auto RandomAccessIteratorContainer::at(key_type const &key) const
    -> mapped_type const &
{
    return m_cont.at(key);
}

auto RandomAccessIteratorContainer::operator[](key_type const &key)
    -> mapped_type &
{
    return m_cont[key];
}

auto RandomAccessIteratorContainer::clear() -> void
{
    throw std::runtime_error("Unimplemented");
}

auto RandomAccessIteratorContainer::find(key_type const &key) -> iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type>>{
            new RandomAccessIterator(m_cont.find(key))});
}
auto RandomAccessIteratorContainer::find(key_type const &key) const
    -> const_iterator
{
    return OpaqueSeriesIterator(
        std::unique_ptr<DynamicSeriesIterator<value_type const>>{
            new RandomAccessIterator(m_cont.find(key))});
}

auto RandomAccessIteratorContainer::contains(key_type const &key) const -> bool
{
    return m_cont.contains(key);
}
} // namespace openPMD
