#include "openPMD/snapshots/ContainerImpls.hpp"
#include "openPMD/Error.hpp"
#include "openPMD/IO/Access.hpp"
#include "openPMD/auxiliary/Variant.hpp"
#include "openPMD/snapshots/ContainerTraits.hpp"
#include "openPMD/snapshots/IteratorHelpers.hpp"
#include "openPMD/snapshots/RandomAccessIterator.hpp"
#include "openPMD/snapshots/StatefulIterator.hpp"
#include <cassert>
#include <memory>
#include <optional>
#include <stdexcept>

namespace openPMD
{
StatefulSnapshotsContainer::StatefulSnapshotsContainer(
    std::variant<std::function<StatefulIterator *()>, StatefulIterator *> begin)
    : members{std::move(begin)}
{}

StatefulSnapshotsContainer::StatefulSnapshotsContainer(
    StatefulSnapshotsContainer const &other) = default;
StatefulSnapshotsContainer::StatefulSnapshotsContainer(
    StatefulSnapshotsContainer
        &&other) noexcept(noexcept(Members(std::declval<Members &&>()))) =
    default;
StatefulSnapshotsContainer &StatefulSnapshotsContainer::operator=(
    StatefulSnapshotsContainer const &other) = default;
StatefulSnapshotsContainer &StatefulSnapshotsContainer::
operator=(StatefulSnapshotsContainer &&other) noexcept(noexcept(
    std::declval<Members>().operator=(std::declval<Members &&>()))) = default;

auto StatefulSnapshotsContainer::get() -> StatefulIterator *
{
    return std::visit(
        auxiliary::overloaded{
            [this](
                std::function<StatefulIterator *()> &deferred_initialization) {
                auto it = deferred_initialization();
                this->members.m_bufferedIterator = it;
                return it;
            },
            [](StatefulIterator *it) { return it; }},
        members.m_bufferedIterator);
}
auto StatefulSnapshotsContainer::get() const -> StatefulIterator const *
{
    return std::visit(
        auxiliary::overloaded{
            [](std::function<StatefulIterator *()> const &)
                -> StatefulIterator const * {
                throw std::runtime_error(
                    "[StatefulSnapshotscontainer] Initialization has been "
                    "deferred, but container is accessed as const, so cannot "
                    "initialize.");
            },
            [](StatefulIterator const *it) { return it; }},
        members.m_bufferedIterator);
}

auto StatefulSnapshotsContainer::stateful_to_opaque(StatefulIterator const &it)
    -> OpaqueSeriesIterator<value_type>
{
    return from_concrete_iterator<StatefulIterator, value_type>(it);
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
        return std::nullopt;
    }
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
        return std::nullopt;
    }
}

StatefulSnapshotsContainer::~StatefulSnapshotsContainer() = default;

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
    /*
     * This should return the sum over all IO steps, counting the number of
     * snapshots contained in each step. This information should be tracked in
     * future in order to have knowledge on where to find an Iteration once it
     * was seen.
     */
    throw std::runtime_error(
        "[StatefulSnapshotsContainer::size()] Unimplemented");
}

auto StatefulSnapshotsContainer::at(key_type const &key) const
    -> mapped_type const &
{
    auto it = get();
    auto current_iteration = it->peekCurrentlyOpenIteration();
    if (!current_iteration.has_value() || (*current_iteration)->first != key)
    {
        throw std::out_of_range(
            "[StatefulSnapshotsContainer::at()] Cannot skip to a Snapshot that "
            "is currently not active in a const context.");
    }
    return (*current_iteration)->second;
}
auto StatefulSnapshotsContainer::at(key_type const &key) -> mapped_type &
{
    auto base_iterator = get();
    auto &result =
        base_iterator->seek({StatefulIterator::Seek::Seek_Iteration_t{key}});
    if (result.is_end())
    {
        throw std::out_of_range(
            "[StatefulSnapshotsContainer::at()] Cannot (yet) skip to "
            "a Snapshot from an I/O step that is not active.");
    }
    return result->second;
}

auto StatefulSnapshotsContainer::operator[](key_type const &key)
    -> mapped_type &
{
    auto base_iterator = get();
    auto &shared = base_iterator->m_data;
    if (!shared)
    {
        throw error::WrongAPIUsage(
            "[WriteIterations] Trying to access after closing Series.");
    }
    auto &optional = *shared;
    if (!optional.has_value())
    {
        throw error::WrongAPIUsage(
            "[WriteIterations] Trying to access after closing Series.");
    }
    auto &s = optional.value();
    auto access = s.series.IOHandler()->m_frontendAccess;

    if (access == Access::READ_WRITE)
    {
        throw std::runtime_error("Stateful iteration on a read-write Series.");
    }
    else if (
        access::read(access) ||
        s.series.iterations.find(key) != s.series.iterations.end())
    {
        return at(key);
    }

    assert(access::write(access));

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

    // create new
    auto &res = s.series.iterations[key];
    Iteration::BeginStepStatus status = [&]() {
        try
        {
            return res.beginStep(/* reread = */ false);
        }
        catch (error::OperationUnsupportedInBackend const &)
        {
            s.series.iterations.retrieveSeries()
                .get()
                .m_currentlyActiveIterations.clear();
            throw;
        }
    }();
    res.setStepStatus(StepStatus::DuringStep);

    s.currentStep.map_during_t(
        [&](detail::CurrentStep::During_t &during) {
            switch (status.stepStatus)
            {
            case AdvanceStatus::OK:
                ++during.step_count;
                during.available_iterations_in_step = {key};
                break;
            case AdvanceStatus::RANDOMACCESS:
                during.available_iterations_in_step.emplace_back(key);
                break;
            case AdvanceStatus::OVER:
                throw error::Internal(
                    "Backend reported OVER status while trying to create "
                    "new Iteration.");
            }
            base_iterator->get().seen_iterations[key] = during.step_count;
            during.iteration_idx = key;
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

    return res;
}

auto StatefulSnapshotsContainer::clear() -> void
{
    throw std::runtime_error(
        "[StatefulSnapshotsContainer::clear()] Unimplemented");
}

auto StatefulSnapshotsContainer::find(key_type const &) -> iterator
{
    throw error::WrongAPIUsage(
        "[StatefulSnapshotsContainer::find] `find()` not available in stateful "
        "iteration as there is only one shared iterator per Series and "
        "`find()` would need to modify that.");
}
auto StatefulSnapshotsContainer::find(key_type const &) const -> const_iterator
{
    throw error::WrongAPIUsage(
        "[StatefulSnapshotsContainer::find] `find()` not available in stateful "
        "iteration as there is only one shared iterator per Series and "
        "`find()` would need to modify that.");
}

auto StatefulSnapshotsContainer::contains(key_type const &) const -> bool
{
    throw std::runtime_error("Unimplemented");
}

auto StatefulSnapshotsContainer::erase(key_type const &) -> size_type
{
    throw std::runtime_error(
        "[StatefulSnapshotsContainer::erase()] Unimplemented");
}
auto StatefulSnapshotsContainer::erase(iterator) -> iterator
{
    throw std::runtime_error(
        "[StatefulSnapshotsContainer::erase()] Unimplemented");
}

auto StatefulSnapshotsContainer::emplace(value_type &&)
    -> std::pair<iterator, bool>
{
    throw std::runtime_error(
        "[StatefulSnapshotsContainer::emplace()] Unimplemented");
}

auto StatefulSnapshotsContainer::snapshotWorkflow() const -> SnapshotWorkflow
{
    return SnapshotWorkflow::Synchronous;
}

RandomAccessIteratorContainer::RandomAccessIteratorContainer(
    Container<Iteration, key_type> cont)
    : m_cont(std::move(cont))
{}

RandomAccessIteratorContainer::~RandomAccessIteratorContainer() = default;

RandomAccessIteratorContainer::RandomAccessIteratorContainer(
    RandomAccessIteratorContainer const &other) = default;
RandomAccessIteratorContainer::RandomAccessIteratorContainer(
    RandomAccessIteratorContainer &&other) noexcept = default;
RandomAccessIteratorContainer &RandomAccessIteratorContainer::operator=(
    RandomAccessIteratorContainer const &other) = default;
RandomAccessIteratorContainer &RandomAccessIteratorContainer::operator=(
    RandomAccessIteratorContainer &&other) noexcept = default;

auto RandomAccessIteratorContainer::currentIteration() const
    -> std::optional<value_type const *>
{
    for (auto begin = m_cont.rbegin(); begin != m_cont.rend(); ++begin)
    {
        if (!begin->second.closed())
        {
            return std::make_optional<value_type const *>(&*begin);
        }
    }
    return std::nullopt;
}

auto RandomAccessIteratorContainer::begin() -> iterator
{
    return from_concrete_iterator<concrete_iterator_type, iterator::value_type>(
        m_cont.begin());
}
auto RandomAccessIteratorContainer::end() -> iterator
{
    return from_concrete_iterator<concrete_iterator_type, iterator::value_type>(
        m_cont.end());
}
auto RandomAccessIteratorContainer::begin() const -> const_iterator
{
    return from_concrete_iterator<
        concrete_const_iterator_type,
        const_iterator::value_type>(m_cont.begin());
}
auto RandomAccessIteratorContainer::end() const -> const_iterator
{
    return from_concrete_iterator<
        concrete_const_iterator_type,
        const_iterator::value_type>(m_cont.end());
}
auto RandomAccessIteratorContainer::rbegin() -> reverse_iterator
{
    return from_concrete_iterator<
        concrete_reverse_iterator_type,
        reverse_iterator::value_type>(m_cont.rbegin());
}
auto RandomAccessIteratorContainer::rend() -> reverse_iterator
{
    return from_concrete_iterator<
        concrete_reverse_iterator_type,
        reverse_iterator::value_type>(m_cont.rend());
}
auto RandomAccessIteratorContainer::rbegin() const -> const_reverse_iterator
{
    return from_concrete_iterator<
        concrete_const_reverse_iterator_type,
        const_reverse_iterator::value_type>(m_cont.rbegin());
}
auto RandomAccessIteratorContainer::rend() const -> const_reverse_iterator
{
    return from_concrete_iterator<
        concrete_const_reverse_iterator_type,
        const_reverse_iterator::value_type>(m_cont.rend());
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
    return from_concrete_iterator<concrete_iterator_type, iterator::value_type>(
        m_cont.find(key));
}
auto RandomAccessIteratorContainer::find(key_type const &key) const
    -> const_iterator
{
    return from_concrete_iterator<
        concrete_const_iterator_type,
        const_iterator::value_type>(m_cont.find(key));
}

auto RandomAccessIteratorContainer::contains(key_type const &key) const -> bool
{
    return m_cont.contains(key);
}

auto RandomAccessIteratorContainer::erase(key_type const &key) -> size_type
{
    return m_cont.erase(key);
}
auto RandomAccessIteratorContainer::erase(iterator it) -> iterator
{
    auto bare_iterator = it.to_concrete_iterator<concrete_iterator_type>();
    if (!bare_iterator.has_value())
    {
        throw std::runtime_error(
            "[RandomAccessIteratorContainer] Illegal dynamic iterator type.");
    }
    return from_concrete_iterator<concrete_iterator_type, iterator::value_type>(
        m_cont.erase(bare_iterator->m_it));
}

auto RandomAccessIteratorContainer::emplace(value_type &&value)
    -> std::pair<iterator, bool>
{
    auto [tmp_iterator, newly_emplaced] = m_cont.emplace(std::move(value));
    return std::make_pair(
        from_concrete_iterator<concrete_iterator_type, iterator::value_type>(
            tmp_iterator),
        newly_emplaced);
}

auto RandomAccessIteratorContainer::snapshotWorkflow() const -> SnapshotWorkflow
{
    return SnapshotWorkflow::RandomAccess;
}

} // namespace openPMD
